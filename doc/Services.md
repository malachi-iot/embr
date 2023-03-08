# Services

## Runtime

`runtime` or `host` is the portion of the service in which notifications can occur.
It has the following characteristics:

* Responsible for managing `TSubject` notification state, if any.
* Inherits Impl as a base class, except in edge case scenarios:
    * Where it's a Impl reference wrapper
    * Where no Impl state is needed  (like a filter or sparse service)
* MUST be stateless except for aforementioned `impl` and `TSubject`

## Impl

This is the core logic, data storage and property declaration area of one's service

## Properties

## Sparse Properties

## Events / Notifications

Events and notifications are an implementation of either properties or more direct usage of underlying subject/observer

Properties have two particular events associated with them:

* `PropertyChanging` (aliased out to `changing` from within runtimes)
* `PropertyChanged` (aliases out to `changed`` from within runtimes)

### State

Services are expected to notify others of their state.  These are minimal and are:

* Stopped
* Running
* Error

### Substate

Substates contain the detail as to the nature of aforementioned `State`.
For example, `Stopped` could be `Unstarted` or `Finished`

### Configuration

These are events fired typically during service startup.  Listeners can
see if the configuration matches their expectation and act accordingly.

### Progress

These are events fired as a percentage of progress for this particular service's
initialization completion.

## Sparse Services

Behave similarly to regular services, but do not track their own `State` and `Substate`
Useful when wrapping up other system mechanisms which track it in their own way.

One can still implement state() and substate() methods to translate

## Convention

### on_xxx

These are impl-pattern support for corresponding service behaviors:

* on_starting, on_start correspond to start
* on_stop corresponds to stop

This is where you implement your own handlers for starting/stopping
Typically one implements these in `runtime` to take advantage of notifications,
but if notifications are not used, it's encouraged to place them into impl.

## Macros

### EMBR_SERVICE_RUNTIME_BEGIN / EMBR_SERVICE_RUNTIME_END

These denote the 'runtime' portion of a service.  _BEGIN takes a parameter, 
the TBD
