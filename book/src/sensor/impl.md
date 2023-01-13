# Sensor C++ implementation

This chapter presents the design of the c++ implementation of the
sensor. It uses the common library for net communication, and some
other utilities. It is link to the libpfm library, in the
`patch_libpfm` directory, that is a patch of the libpfm to manage MSR
events.

The idea of the following sub chapters is to present an overview of
the implementation, so that the relationships between classes and
functions are easier to understand, but does not aim to present the
full documentation (i.e. function prototypes etc.).

