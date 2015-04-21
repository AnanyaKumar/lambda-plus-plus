# TODO

- get boilerplate MPI code working
  - simple "Hello, world!" application on GHC machines
- build Sequence library up from boilerplace MPI code
  - Doesn't have to be super optimized just yet
  - should contain map, reduce, scan, sort
  - at this point, make sure the overall API of the library won't change too much
- start implementing algorithms using the Sequence library
- Optimize
  - write hardware autoconfig
  - fine tune applications of Sequence library
  - fine tune implementation of Sequence library


## Refactor Sequence implementation

We should refactor the Sequence implementation according to the following
inheritance scheme:

- `class Sequence` (abstract class, like
  <http://www.tutorialspoint.com/cplusplus/cpp_interfaces.htm>)
  - Files: `sequence.h`
- `class ParallelSequence` extends Sequence
  - `parallel_sequence.{h,cpp}`
- `class SerialSequence` extends Sequence
  - `serial_sequence.{h,cpp}`

