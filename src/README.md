Unit Test in ABC
===
[Catch2](https://github.com/catchorg/Catch2) is a C++ Test Framework with only one Header file. It provides simple API to write unit test.

Here is a simple way to add Catch2 into ABC without modified any file in the original ABC project.

These files are implemented by Kuan-Hua Tu, 2018/04/20

## How to install
1. Put the folder ```extUnitTest/``` under ```src/``` in an ABC project.
2. Use ```make test``` as compiling ABC
3. It should show the info like this:
```
./abc -q "utest"
===============================================================================
No tests ran
```

## After install
A new commend ```utest``` is added into ABC to run the test suit.
You can use ```make test``` in terminal to compile and check all test cases.
You also can use ```./abc -q "utest"``` to check all test caseswith compiled abc.

## How to write test cases
- You can start with the [tutorial](https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md#top). 
- Or you can take the ```extExample/``` as a simple example.
  1. Put the folder ```extExample/``` under ```src/``` in an ABC project. 
  2. Type ```make test``` in terminal.
  3. It should show the info like this:
  ```
  ./abc -q "utest"
  
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  utest is a Catch v2.2.2 host application.
  Run with -? for options
  
  -------------------------------------------------------------------------------
  Catch2 works if this test failed as expected
  -------------------------------------------------------------------------------
  src/extExample/example_test.cc:48
  ...............................................................................
  
  src/extExample/example_test.cc:50: FAILED:
    CHECK( false )
  
  src/extExample/example_test.cc:51: FAILED:
    REQUIRE( false )
  
  ===============================================================================
  test cases: 2 | 1 passed | 1 failed
  assertions: 9 | 7 passed | 2 failed
  ```
