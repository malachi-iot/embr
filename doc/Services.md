# Services

## Runtime

`runtime` or `host` is the portion of the service in which notifications can occur.
It has the following characteristics:

* Responsible for managing `TSubject` notification state, if any.
* Inhereits Impl as a base class, except in edge case scenarios where it's a Impl reference wrapper
* MUST be stateless except for aforementioned `impl` and `TSubject`

## Impl

This is the core logic, data storage and property declaration area of one's service

## Properties

## Sparse Properties

## Sparse Services
