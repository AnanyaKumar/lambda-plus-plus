# TODO

- Software Engineering
  - Structure the code better
  - Folders: sequence, util, unittest, perftest, demo, cluster

- Basic Functionality
  - Fix warnings
    - It looks like we're doing our constructors incorrectly because of the
      abstract, virtual class issue. We get the following errors that seem to
      indicate that our destructors aren't being called:
    - When compiling:

      ```gcc
      src/mandelbrot.cpp: In function ‘void test_mandelbrot()’:
      src/mandelbrot.cpp:107:12: warning: deleting object of abstract class type ‘Sequence<int>’ which has non-virtual destructor will cause undefined behaviour [-Wdelete-non-virtual-dtor]
           delete seq;
                  ^</int>
      ```
    - When running:

      ```
      [ghc27.ghc.andrew.cmu.edu:19916] WARNING: There were 1 Windows created but not freed.
      ```


- Demos/Applications
  - Think about how to implement sorting (doesn't seem trivial at all)
  - Implement DP algorithms
  - Write high performance sequential benchmarks for algorithms (for an honest comparison, we shouldn't be using the sequential Sequence class (because that could have overheads).
  - Test our algorithms on Blacklight

- Optimize the parallel sequence library
  - Distribute work better
    - Chunking + randomized distribution of chunks
    - User specified work distributions
    - Work stealing (for cases where some computations take much longer than others)
  - Customize based on hardware configuration
    - Change work allocation if some cores are slow
    - If communication costs are high, then don't have too many chunks

- Advertising (very important, we need to show our good ideas)
  - Figure out how to market our platform
  - Update website based on marketing

