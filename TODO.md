# TODO

- Software Engineering
  - Structure the code better
  - Folders: sequence, util, unittest, perftest, demo, cluster

- Basic Functionality
  - Implement "set" in the parallel sequence library

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

