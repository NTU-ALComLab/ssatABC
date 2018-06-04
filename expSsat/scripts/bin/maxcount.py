# MaxCount 1.0.0
# An approximate Max#SAT solver
# Written by Daniel Fremont and Markus Rabe
# This algorithm is explained in the paper "Maximum Model Counting"
# by Daniel J. Fremont, Markus N. Rabe, and Sanjit A. Seshia, AAAI 2017

import sys
import os
import math
import random
import pycryptosat
import time
import itertools
import argparse

### parse command-line arguments

parser = argparse.ArgumentParser(description='Approximately solve a Max#SAT problem.', usage='%(prog)s [-h] [options] formula k', formatter_class=argparse.ArgumentDefaultsHelpFormatter)

## required arguments

parser.add_argument('inputFilename', help='Max#SAT problem to solve, in extended DIMACS format', metavar='formula')
parser.add_argument('k', help='number of copies of the formula in the self-composition', type=int)

## optional arguments

# general
parser.add_argument('--seed', help='random seed to use', metavar='s', type=int, default=0)
parser.add_argument('--runIndex', help='number included in temporary file names (useful for running multiple instances simultaneously)', metavar='r', type=int, default=0)
parser.add_argument('--verbosity', help='information to output: 0 = max-count estimate and witness; 1 = also max-count bounds; 2 = everything', metavar='level', type=int, choices=[0,1,2], default=2)
parser.add_argument('--scalmc', help='path to scalmc binary', metavar='path', default='./scalmc')

# sampling
parser.add_argument('--samples', help='number of samples to generate from self-composition', metavar='n', type=int, default=20)
parser.add_argument('--samplingKappa', help='kappa parameter for UniGen2 (determines sampling tolerance; kappa=0.5782 corresponds to epsilon=16)', metavar='kappa', type=float, default=0.5782)
parser.add_argument('--multisample', help='return multiple samples from each UniGen2 call; may yield better results than correspondingly increasing --samples, but the tool does not currently take full advantage of this', action='store_true')

# counting
parser.add_argument('--countingTolerance', help='counting tolerance', metavar='epsilon', type=float, default=1)
parser.add_argument('--upperBoundConfidence', help='minimum confidence for upper bound', metavar='uconf', type=float, default=0.6)
parser.add_argument('--lowerBoundConfidence', help='minimum confidence for lower bound', metavar='lconf', type=float, default=0.8)
parser.add_argument('--monteCarloSamples', help='number of Monte Carlo samples for counting', metavar='n', type=int, default=2000)
parser.add_argument('--enumerationThreshold', help='maximum number of solutions to enumerate for exact counting', metavar='n', type=int, default=256)

# refinement (disabled by default)
parser.add_argument('--refine', help='refine count of best sample obtained', action='store_true')
parser.add_argument('--refinementTolerance', help='counting tolerance for refinement', metavar='epsilon', type=float, default=0.4142)
parser.add_argument('--refinementMCSamples', help='number of Monte Carlo samples for refinement', metavar='n', type=int, default=20000)
parser.add_argument('--refinementEnumThreshold', help='maximum number of solutions to enumerate for exact refinement', metavar='n', type=int, default=1024)

## parse and validate arguments

args = parser.parse_args()

inputFilename = args.inputFilename
if ' ' in inputFilename:
	raise Exception('the input file cannot have spaces in its name')

seed = args.seed
runIndex = args.runIndex
verbosity = args.verbosity
scalmcPath = args.scalmc

k = args.k
if k < 0:
	raise Exception('k must be nonnegative')

numSamples = args.samples
kappa = args.samplingKappa
useMultisampling = args.multisample
if numSamples < 1:
	raise Exception('number of samples must be positive')
if kappa <= 0 or kappa >= 1:
	raise Exception('samplingKappa must be strictly between 0 and 1')

upperBoundConfidence = args.upperBoundConfidence
countConfidence = args.lowerBoundConfidence
if upperBoundConfidence <= 0 or upperBoundConfidence >= 1:
	raise Exception('upperBoundConfidence must be strictly between 0 and 1')
if countConfidence <= 0 or countConfidence >= 1:
	raise Exception('lowerBoundConfidence must be strictly between 0 and 1')
if upperBoundConfidence >= countConfidence:
	# the correctness of the upper bound depends on that of the count, so countConfidence
	# (which is the confidence of the lower bound) must be greater than upperBoundConfidence
	#raise Exception('lowerBoundConfidence must be strictly greater than upperBoundConfidence')
	countConfidence = 1 - ((1 - upperBoundConfidence) / 2)	# heuristic

countEpsilon = args.countingTolerance
countEnumerationThreshold = args.enumerationThreshold
monteCarloSamples = args.monteCarloSamples
if countEpsilon <= 0:
	raise Exception('countingTolerance must be positive')

useRefinement = args.refine
refinementEpsilon = args.refinementTolerance
monteCarloRefinementSamples = args.refinementMCSamples
refinementEnumerationThreshold = args.refinementEnumThreshold

### initialization

startTime = time.time()

# function for printing only at a certain level of verbosity
def printV(verbosityLevel, text, withNewline=True):
	if verbosity >= verbosityLevel:
		if withNewline:
			print(text)
		else:
			sys.stdout.write(text)

random.seed(seed)
printV(2, 'c Using random seed %d, runIndex %d' % (seed, runIndex))

# temporary file names
kfoldFilename = inputFilename+'.'+str(runIndex)+'.kfold'
sampleFilename = inputFilename+'.'+str(runIndex)+'.samples'
outputFilename = inputFilename+'.'+str(runIndex)+'.out'
onefoldFilename = inputFilename+'.'+str(runIndex)+'.1fold'
countFilename = inputFilename+'.'+str(runIndex)+'.count'
cuspLogFilename = inputFilename+'.'+str(runIndex)+'.log'

## sampling parameters

# the formula for sampleEpsilon depends on the choices of
# samplingPivotAC and samplingTApproxMC - see the UniGen2 paper (TACAS 2015)
#samplingPivotAC = 73
#samplingTApproxMC = 67
#kappa = 0.638
#sampleEpsilon = ((1 + kappa) * (7.44 + (0.392 / ((1 - kappa) * (1 - kappa))))) - 1
samplingPivotAC = 73
samplingTApproxMC = 1
sampleEpsilon = ((1 + kappa) * (8.227 + (0.453 / ((1 - kappa) * (1 - kappa))))) - 1

# multisampling will generate more samples, but not correspondingly improve the
# theoretical guarantees (currently)
if useMultisampling:
	uniGenPivot = math.ceil(4.03 * (1 + (1/kappa)) * (1 + (1/kappa)))
	uniGenLoThresh = int(uniGenPivot / (1.41 * (1 + kappa)))
	uniGenNumSamples = numSamples * uniGenLoThresh
else:
	uniGenNumSamples = numSamples

## counting parameters

refinementFailureProb = (1.0 - countConfidence) / (uniGenNumSamples + 1)	# heuristic
refinementConfidence = 1.0 - refinementFailureProb

if useRefinement:
	perSampleCountingFailure = ((1.0 - countConfidence) - refinementFailureProb) / uniGenNumSamples
	if perSampleCountingFailure <= 0:
		raise Exception('refinementConfidence must be greater than countConfidence')
else:
	perSampleCountingFailure = (1.0 - countConfidence) / uniGenNumSamples
perSampleCountingConfidence = 1 - perSampleCountingFailure

monteCarloDensityConfidence = 1 - (perSampleCountingFailure / 2)	# heuristic
if monteCarloDensityConfidence <= perSampleCountingConfidence:
	raise Exception('monteCarloDensityConfidence must be strictly greater than perSampleCountingConfidence')
monteCarloSampleCountingConfidence = perSampleCountingConfidence / monteCarloDensityConfidence
monteCarloDensityGranularity = math.sqrt(-math.log(1 - monteCarloDensityConfidence) / (2 * monteCarloSamples)) if monteCarloSamples > 0 else 0

# some versions of Python lack math.log2
try:
	log2 = math.log2
except AttributeError:
	log2 = lambda x: math.log(x, 2)

# For greater reproducibility, we don't use any of the derived random number
# generation functions in Python (they've changed before, e.g. in 3.2).
randbool = lambda: random.random() < 0.5

### parse formula and extract maximization/counting variables

printV(2, 'c Parsing formula...')

maxVars = []
seenMaxVars = set()
countingVars = set()
with open(inputFilename, 'r') as inputFile:
	for (lineNumber, line) in enumerate(inputFile, start=1):
		if line[:6] == 'c max ':
			fields = line.split()[2:]
			if fields[-1] != '0':
				raise Exception('Malformed maximization variable comment on line '+str(lineNumber))
			try:
				for field in fields[:-1]:
					var = int(field)
					if var not in seenMaxVars:
						maxVars.append(var)
						seenMaxVars.add(var)
			except ValueError:
				raise Exception('Non-integer maximization variable on line '+str(lineNumber))
		elif line[:6] == 'c ind ':
			fields = line.split()[2:]
			if fields[-1] != '0':
				raise Exception('Malformed counting variable comment on line '+str(lineNumber))
			try:
				for field in fields[:-1]:
					countingVars.add(int(field))
			except ValueError:
				raise Exception('Non-integer counting variable on line '+str(lineNumber))
numCountingVars = len(countingVars)

printV(2, 'c Formula has %d maximization and %d counting variables' % (len(maxVars), numCountingVars))

### sample from assignments to maximization variables

printV(2, 'c Generating %d independent samples from %d-fold self-composition' % (numSamples, k))

if useMultisampling:
	printV(2, 'c Using multisampling: %d total samples' % uniGenNumSamples)

def sampleFromSelfComposition():
	# construct k-fold self-composition
	os.system('python selfcomposition.py '+str(k)+' '+inputFilename+' > '+kfoldFilename)

	# generate samples
	printV(2, 'c Sampling with tolerance (1+%f)... ' % sampleEpsilon, False)
	sys.stdout.flush()

	timepoint = time.time()
	sampleCommand = scalmcPath+' --cuspLogFile='+cuspLogFilename
	sampleCommand += ' --multisample='+('1' if useMultisampling else '0')
	sampleCommand += ' --random='+str(seed)
	sampleCommand += ' --pivotAC='+str(samplingPivotAC)
	sampleCommand += ' --tApproxMC='+str(samplingTApproxMC)
	sampleCommand += ' --kappa='+str(kappa)
	sampleCommand += ' --samples='+str(uniGenNumSamples)
	sampleCommand += ' --sampleFile='+sampleFilename
	sampleCommand += ' '+kfoldFilename+' > '+outputFilename
	os.system(sampleCommand)
	printV(2, 'completed in %d s' % (time.time() - timepoint))

	# parse samples
	samples = set()
	try:
		formulaUnsat = False
		with open(outputFilename, 'r') as outputFile:
			for line in outputFile:
				if line == 'The input formula is unsatisfiable.\n':
					formulaUnsat = True
					break
		if not formulaUnsat:
			with open(sampleFilename, 'r') as sampleFile:
				for line in sampleFile:
					fields = line[1:].split(':')[0].split()
					if fields[-1] != '0':
						raise Exception('Malformed sample file from scalmc')
					literals = set(map(lambda f: int(f), fields[:-1]))
					sample = []
					for var in maxVars:
						if var in literals:
							sample.append(var)
						elif -var in literals:
							sample.append(-var)
						else:
							raise Exception('Incomplete sample from scalmc')
					samples.add(tuple(sample))
	except IOError as e:
		raise Exception('Sampling did not complete successfully')

	return samples

samples = set()
if k == 0:	# sample uniformly from assignments to maximization variables
	for i in range(numSamples):
		sample = []
		for var in maxVars:
			if randbool():
				sample.append(var)
			else:
				sample.append(-var)
		samples.add(tuple(sample))
else:
	samples = sampleFromSelfComposition()

printV(2, 'c Obtained %d distinct assignments to maximization variables' % len(samples))

if len(samples) == 0:
	printV(0, 'c Formula is UNSAT')
	printV(0, 'c Estimated max-count: 0 x 2^0')
	printV(1, 'c Max-count is <= 0 x 2^0 with probability >= 1')
	printV(1, 'c Max-count is >= 0 x 2^0 with probability >= 1')
	printV(2, 'c Total runtime %d s' % (time.time() - startTime))
	sys.exit(0)

### count solutions for each sample

# construct 1-fold self-composition and extract clauses
os.system('python selfcomposition.py 1 '+inputFilename+' > '+onefoldFilename)

clauses = []
with open(onefoldFilename, 'r') as onefoldFile:
	for line in onefoldFile:
		if line[0] == '-' or line[0].isdigit():
			clause = list(map(lambda f: int(f), line.split()[:-1]))
			clauses.append(clause)

def countSampleWithHashing(sample, epsilon=countEpsilon, confidence=perSampleCountingConfidence):
	pivotAC = int(math.ceil(9.84 * (1 + (epsilon / (1.0 + epsilon))) * (1 + (1.0/epsilon)) * (1 + (1.0/epsilon))))
	tApproxMC = int(math.ceil(17 * log2(3.0 / (1 - confidence))))
	countCommand = scalmcPath+' --cuspLogFile='+cuspLogFilename
	countCommand += ' --random='+str(seed)
	countCommand += ' --pivotAC='+str(pivotAC)
	countCommand += ' --tApproxMC='+str(tApproxMC)
	countCommand += ' '+countFilename+' > '+outputFilename
	# assign maximization variables by adding unit clauses
	os.system('cp '+onefoldFilename+' '+countFilename)
	with open(countFilename, 'a') as countFile:
		for literal in sample:
			countFile.write(str(literal)+' 0\n')
	# count resulting formula
	os.system(countCommand)
	cellCount = -1
	hashCount = -1
	with open(outputFilename, 'r') as outputFile:
		for line in outputFile:
			if line[:24] == 'Number of solutions is: ':
				if cellCount >= 0:
					raise Exception('Malformed output from scalmc')
				fields = line[24:].split('x')
				cellCount = int(fields[0])
				hashCount = int(fields[1][3:])
			elif line == 'The input formula is unsatisfiable.\n':
				if cellCount >= 0:
					raise Exception('Malformed output from scalmc')
				cellCount = 0
				hashCount = 0
	if cellCount == -1:
		raise Exception('scalmc did not complete successfully')

	sampleMant = float(cellCount)
	sampleExp = hashCount
	while sampleMant >= 2:
		sampleMant /= 2
		sampleExp += 1

	return (sampleMant, sampleExp, hashCount == 0)

def solverForSample(sample):
	# set up solver with clauses from original formula
	solver = pycryptosat.Solver()
	for clause in clauses:
		solver.add_clause(clause)

	# assert the assignment to the maximization variables
	for literal in sample:
		solver.add_clause([literal])

	return solver

def countSampleWithBruteForce(sample):
	solver = solverForSample(sample)

	# if formula is UNSAT, don't bother trying assignments
	sat, model = solver.solve()
	if not sat:
		return (0, 0, True)

	count = 0
	for assignment in itertools.product(*([v,-v] for v in countingVars)):
		# check if assignment is consistent
		sat, model = solver.solve(assignment)
		if sat:
			count += 1

	sampleMant = float(count)
	sampleExp = 0
	while sampleMant >= 2:
		sampleMant /= 2
		sampleExp += 1

	return (sampleMant, sampleExp, True)

def countSampleWithEnumeration(sample, threshold):
	solver = solverForSample(sample)

	# enumerate assignments until UNSAT or threshold exceeded
	count = 0
	while True:
		sat, model = solver.solve()
		if not sat:
			break
		count += 1
		if count > threshold:
			break
		blockingClause = []
		for var in countingVars:
			if model[var]:
				blockingClause.append(-var)
			else:
				blockingClause.append(var)
		solver.add_clause(blockingClause)

	if count > threshold:
		return (0, 0, False)

	sampleMant = float(count)
	sampleExp = 0
	while sampleMant >= 2:
		sampleMant /= 2
		sampleExp += 1

	return (sampleMant, sampleExp, True)

def countSampleWithMonteCarlo(sample, numMCSamples=monteCarloSamples):
	solver = solverForSample(sample)

	# if formula is UNSAT, don't bother sampling
	sat, model = solver.solve()
	if not sat:
		return (0, 0, True)

	# take samples
	positiveSamples = 0
	for i in range(numMCSamples):
		# generate random assignment to the counting variables
		assumptions = []
		for var in countingVars:
			if randbool():
				assumptions.append(var)
			else:
				assumptions.append(-var)

		# check if assignment is consistent
		sat, model = solver.solve(assumptions)
		if sat:
			positiveSamples += 1

	sampleMant = float(positiveSamples) / numMCSamples
	sampleExp = numCountingVars
	while sampleMant > 0 and sampleMant < 1:
		sampleMant *= 2
		sampleExp -= 1

	return (sampleMant, sampleExp, False)

def binarySearch(start, direction):
	best = start
	worst = 0
	current = (worst + best) / 2
	done = False
	while True:
		d = direction(current, worst, best)
		if d == 0:
			return current
		elif d == 1:
			worst = current
			current = (current + best) / 2
		elif d == -1:
			best = current
			current = (worst + current) / 2
		else:
			raise Exception('Mr. Fremont shoots himself in the foot yet again (this should be impossible)')

def monteCarloFailureBound(density, epsilon, numMCSamples=monteCarloSamples):
	if density == 0:
		return 1

	multError = 1 + epsilon
	dme = density * multError
	if dme < 1:
		upperProb = ((1 - density) / (1 - dme)) ** (1 - dme)
		upperProb /= multError ** dme
	elif dme == 1:
		upperProb = density
	else:
		upperProb = 0

	dde = density / multError
	if dde > 0:
		lowerProb = (density / dde) ** dde
		lowerProb *= ((1 - density) / (1 - dde)) ** (1 - dde)
	else:
		lowerProb = 1 - density

	return ((upperProb ** numMCSamples) + (lowerProb ** numMCSamples))

def monteCarloEpsilon(density, numMCSamples=monteCarloSamples, confidence=monteCarloSampleCountingConfidence):
	if density == 0:
		return float('inf')

	startEpsilon = float(countEpsilon)
	def epsilonSearcher(failureBound):
		if failureBound(density, startEpsilon, numMCSamples) >= 1 - confidence:
			return float('inf')
		else:
			def direction(eps, worst, best):
				if best - worst < 0.000001:
					return 0
				if failureBound(density, eps, numMCSamples) < 1 - confidence:
					return -1
				else:
					return 1

			return binarySearch(startEpsilon, direction)

	return epsilonSearcher(monteCarloFailureBound)

def countSample(sample, label, densityEstimate=None, confidence=perSampleCountingConfidence, epsilon=countEpsilon, numMCSamples=monteCarloSamples, enumerationThreshold=countEnumerationThreshold):
	actualEpsilon = 0
	sampleExact = False

	## estimate density by sampling (unless disabled by using 0 samples)
	# if number of possible assignments is smaller than number
	# of samples, pick systematically instead of randomly
	if numMCSamples == 0:
		densityTooSmall = True
	else:
		densityTooSmall = False
		if densityEstimate != None:
			densityTooSmall = (monteCarloEpsilon(densityEstimate, numMCSamples=numMCSamples) > epsilon)

	if not densityTooSmall:
		if log2(numMCSamples) >= numCountingVars:
			printV(2, 'c Counting %s by brute force... ' % label, False)
			sys.stdout.flush()
			(sampleMant, sampleExp, sampleExact) = countSampleWithBruteForce(sample)
			printV(2, '%.3f x 2^%d' % (sampleMant, sampleExp))
		else:
			printV(2, 'c Counting %s with Monte Carlo... ' % label, False)
			sys.stdout.flush()
			(sampleMant, sampleExp, sampleExact) = countSampleWithMonteCarlo(sample, numMCSamples=numMCSamples)
			printV(2, '%.3f x 2^%d' % (sampleMant, sampleExp))

			# check whether density is too low for estimate to be accurate
			diff = numCountingVars - sampleExp
			if diff > 1000:
				densityEstimate = 0
			else:
				densityEstimate = sampleMant / (2.0 ** diff)

			densityEstimate -= monteCarloDensityGranularity
			if densityEstimate > 0:
				actualEpsilon = monteCarloEpsilon(densityEstimate)
				sampleMultError = 1 + actualEpsilon
				densityTooSmall = (actualEpsilon > epsilon)
			else:
				densityTooSmall = True

	if sampleExact:
		return (sampleMant, sampleExp, sampleExact, 1, 1)

	## unless the number of solutions is known to be too large, attempt enumeration
	if (enumerationThreshold > 0) and (densityTooSmall or (sampleExp <= log2(enumerationThreshold))):
		printV(2, 'c Counting %s with enumeration... ' % label, False)
		sys.stdout.flush()

		(newSampleMant, newSampleExp, newSampleExact) = countSampleWithEnumeration(sample, enumerationThreshold)
		if newSampleExact:
			printV(2, '%.3f x 2^%d' % (newSampleMant, newSampleExp))
			return (newSampleMant, newSampleExp, newSampleExact, 1, 1)
		else:
			printV(2, 'threshold exceeded')

	# if we couldn't use enumeration to improve a Monte Carlo estimate, return that estimate
	if not densityTooSmall:
		return (sampleMant, sampleExp, sampleExact, sampleMultError, perSampleCountingConfidence)

	## fall back on hashing
	printV(2, 'c Counting %s with hashing... ' % label, False)
	sys.stdout.flush()

	(sampleMant, sampleExp, sampleExact) = countSampleWithHashing(sample, epsilon=epsilon, confidence=confidence)
	printV(2, '%.3f x 2^%d' % (sampleMant, sampleExp))
	return (sampleMant, sampleExp, sampleExact, 1 + epsilon, confidence)

printV(2, 'c Using %d Monte Carlo samples, enumeration threshold %d' % (monteCarloSamples, countEnumerationThreshold))
printV(2, 'c Using density confidence %g, granularity %g' % (monteCarloDensityConfidence, monteCarloDensityGranularity))
printV(2, 'c Using hashing tolerance (1+%g) and confidence %g' % (countEpsilon, perSampleCountingConfidence))
timepoint = time.time()

maxMant = -1
maxExp = -1
maxSample = None
maxExact = False	# whether the count for maxSample is exact
maxMultError = -1
actualCountConfidence = 1
worstMultError = 1
for (index, sample) in enumerate(samples):
	label = 'witness '+str(index+1)
	(sampleMant, sampleExp, sampleExact, sampleMultError, sampleConfidence) = countSample(sample, label)

	if not sampleExact:
		actualCountConfidence *= sampleConfidence
	if sampleMultError > worstMultError:
		worstMultError = sampleMultError

	# see if count is larger than the best so far
	if sampleExp > maxExp or (sampleExp == maxExp and sampleMant > maxMant):
		maxMant = sampleMant
		maxExp = sampleExp
		maxSample = sample
		maxExact = sampleExact
		maxMultError = sampleMultError

if actualCountConfidence < countConfidence:
	raise Exception('count confidence calculation corrupted')

printV(2, 'c Counting completed in %d s' % (time.time() - timepoint))

printV(2, 'c Witness with largest estimated count:')
printV(0, 'v ', False)
for literal in maxSample:
	printV(0, str(literal)+' ', False)
printV(0, '0')

if maxExact:
	printV(2, 'c Estimated count for this witness is exact')

### optionally refine count for the best sample

refinedMant = maxMant
refinedExp = maxExp
refinedExact = maxExact
refinedMultError = maxMultError
refinedConfidence = actualCountConfidence
if useRefinement and not maxExact:
	printV(2, 'c Refining lower bound')
	printV(2, 'c Using %d Monte Carlo samples, enumeration threshold %d' % (monteCarloRefinementSamples, refinementEnumerationThreshold))
	printV(2, 'c Using hashing tolerance (1+%g) and confidence %g' % (refinementEpsilon, refinementConfidence))
	
	timepoint = time.time()

	# compute density estimate (to decide whether even to attempt Monte Carlo)
	diff = numCountingVars - refinedExp
	if diff > 1000:
		densityEstimate = 0
	else:
		densityEstimate = refinedMant / (2.0 ** diff)

	label = 'witness'
	(newRefinedMant, newRefinedExp, newRefinedExact, newRefinedMultError, newRefinedConfidence) = countSample(sample, label, densityEstimate=densityEstimate, confidence=refinementConfidence, epsilon=refinementEpsilon, numMCSamples=monteCarloRefinementSamples, enumerationThreshold=refinementEnumerationThreshold)
	printV(2, 'c Refinement completed in %d s' % (time.time() - timepoint))
	printV(2, 'c Refined witness count: %g x 2^%d' % (newRefinedMant, newRefinedExp))

	if newRefinedExp > refinedExp or (newRefinedExp == refinedExp and newRefinedMant >= refinedMant):
		refinedMant = newRefinedMant
		refinedExp = newRefinedExp
		refinedExact = newRefinedExact
		refinedMultError = newRefinedMultError
		refinedConfidence = actualCountConfidence + newRefinedConfidence - 1

### output max-count estimate and bounds

# compute smallest upperMultError such that with at least the desired confidence,
# all estimated counts are within (1+countEpsilon) error and at least one sample
# has count at most a factor of upperMultError less than the maximum
useTrivialUpperBound = False
requiredConfidence = upperBoundConfidence / actualCountConfidence
requiredDelta = 1 - requiredConfidence
f = (1 - (requiredDelta ** (1.0 / numSamples))) * (1 + sampleEpsilon)
if f >= 1:		# cannot do better than trivial bound
	useTrivialUpperBound = True
elif k == 0:
	# compute probability of getting the maximum by sheer luck
	mp = 2.0 ** -len(maxVars)
	mp = 1 - ((1 - mp) ** numSamples)
	if mp >= requiredConfidence:
		upperMultError = 1
	else:
		useTrivialUpperBound = True
else:
	krat = len(maxVars) / k
	if f == 0:	# underflow; use series expansion of log
		logFail = log2(-math.log(requiredDelta)) - log2(numSamples) + (log2(requiredDelta) / (2 * numSamples))
		logMultError = (len(maxVars) - logFail + log2(1 + sampleEpsilon)) / k
		if logMultError > 1000:
			useTrivialUpperBound = True
		else:
			upperMultError = 2.0 ** logMultError
	else:
		fRecip = 1 / f
		if krat > 1000 or krat - (log2(fRecip - 1) / k) > 1000:
			useTrivialUpperBound = True
		else:
			upperMultError = (2.0 ** krat) / ((fRecip - 1) ** (1.0 / k))

## compute upper bound

sampleBoundTrivial = False
if not useTrivialUpperBound:
	upperBoundMant = maxMant * upperMultError
	if not maxExact:
		upperBoundMant *= maxMultError
	upperBoundExp = maxExp

	if log2(maxMultError) + log2(worstMultError) + log2(upperMultError) >= numCountingVars:
		sampleBoundTrivial = True

clampedMaxMant = maxMant
clampedMaxExp = maxExp
if maxExp >= numCountingVars:
	useTrivialUpperBound = True
	clampedMaxMant = 1
	clampedMaxExp = numCountingVars

if useTrivialUpperBound or sampleBoundTrivial:
	diff = numCountingVars - clampedMaxExp
	if diff > 1000:
		quality = 0
	else:
		quality = (clampedMaxMant / (2.0 ** (numCountingVars - clampedMaxExp))) / maxMultError
	printV(2, 'c Witness quality (exact count / max-count) >= %g with probability >= %g' % (quality, actualCountConfidence))
else:
	quality = 1 / (maxMultError * worstMultError * upperMultError)
	printV(2, 'c Witness quality (exact count / max-count) >= %g with probability >= %g' % (quality, upperBoundConfidence))

printV(0, 'c Estimated max-count: %g x 2^%d' % (clampedMaxMant, clampedMaxExp))

if useTrivialUpperBound or (upperBoundMant > 0 and log2(upperBoundMant) + upperBoundExp >= numCountingVars):
	printV(1, 'c Max-count is <= 1 x 2^%d with probability >= 1 (trivial bound)' % numCountingVars)
else:
	while upperBoundMant >= 2:
		upperBoundMant /= 2
		upperBoundExp += 1
	printV(1, 'c Max-count is <= %g x 2^%d with probability >= %g' % (upperBoundMant, upperBoundExp, upperBoundConfidence))

## compute lower bound

lowerBoundMant = refinedMant
if refinedExact:
	lowerBoundConfidence = 1
else:
	lowerBoundConfidence = refinedConfidence
	lowerBoundMant /= refinedMultError
lowerBoundExp = refinedExp
while lowerBoundMant > 0 and lowerBoundMant < 1:
	lowerBoundMant *= 2
	lowerBoundExp -= 1

if lowerBoundMant <= 0:
	printV(1, 'c Max-count is >= 0 x 2^0 with probability >= 1 (trivial bound)')
else:
	printV(1, 'c Max-count is >= %g x 2^%d with probability >= %g' % (lowerBoundMant, lowerBoundExp, lowerBoundConfidence))

printV(2, 'c Total runtime %d s' % (time.time() - startTime))
