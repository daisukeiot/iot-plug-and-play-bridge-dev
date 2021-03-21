# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.


if(NOT ${build_impinj})
  add_subdirectory(console)
  add_subdirectory(service)
else()
  add_subdirectory(impinj)
endif()
