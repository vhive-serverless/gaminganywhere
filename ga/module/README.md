# GamingAnywhere Modules

GamingAnywhere (GA) supports the use of modular components, known as modules, to extend its functionality and support different types of streaming and capturing scenarios. This README provides an overview of how GamingAnywhere modules work and guidelines for implementing them.

## Table of Contents
- [Introduction](#introduction)
- [Module Structure](#module-structure)
- [Module Loading](#module-loading)
- [Module Functions](#module-functions)

## Introduction

GamingAnywhere modules are dynamic libraries that can be loaded at runtime to enhance the capabilities of the system. These modules follow a specific structure and must implement certain functions to be recognized and utilized by GamingAnywhere.

## Module Structure

Each module is a dynamic library (DLL on Windows, shared object on Unix-like systems) with a specific set of functions. The common structure of a module includes:

- **ga_module_t Structure**: This structure defines the functions that a module must implement. It includes pointers to functions such as `init`, `start`, `stop`, `deinit`, `ioctl`, `notify`, `raw`, and `send_packet`. These functions handle various aspects of module initialization, control, and data processing.

- **Module Initialization Function**: The module must provide a function named `module_load` that returns an instance of the `ga_module_t` structure. This function is responsible for initializing and returning the module instance.

## Module Loading

The `ga_load_module` function is responsible for loading a module. It takes the name of the module (excluding the file extension) and returns a pointer to the loaded `ga_module_t` structure. This function automatically appends the appropriate file extension based on the platform.

## Module Functions

The following functions represent the core functionalities that a module can implement:

- **init**: Initializes the module.
- **start**: Starts the module.
- **stop**: Stops the module.
- **deinit**: Deinitializes the module.
- **ioctl**: Handles input/output control operations.
- **notify**: Notifies the module.
- **raw**: Provides access to raw module data.
- **send_packet**: Sends a packet to the module (primarily used by server modules).

Wrapper functions, such as `ga_module_init`, `ga_module_start`, `ga_module_stop`, `ga_module_deinit`, `ga_module_ioctl`, `ga_module_notify`, `ga_module_raw`, and `ga_module_send_packet`, are provided to ensure safe and consistent interaction with these functions.