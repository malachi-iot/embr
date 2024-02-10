# esp-helper esp-idf component

Document v0.1

# Detail

## 1. Design Goals

This component was originally used to bring up tests more easily.
Since then, it has grown into a useful utility for ESP32 app development overall.

### 1.1. Features

#### 1.1.1. WiFi assist

#### 1.1.2. Board Traits

### 1.2. Caveats

Given its lineage, the component is still somewhat clumsy.  In particular:

* Tucked away in embr test area
* Code construction is quite sloppy
* “helper” nomenclature itself is outgrown

## 2. Usage

### 2.1. Inclusion in a project

Clumsily, one must scoop up `components` folder from `test` directory.

### 2.2. Configuring

Component registers itself via `Kconfig`, so regular `esp.py menuconfig` picks it up.

For details, see [Kconfig settings](MenuConfig.md)

## 3. Opinions & Observations

### 3.1. Inclusion into embr itself

As the features become more stable, it’s likely the greater embr
library itself will absorb this helper