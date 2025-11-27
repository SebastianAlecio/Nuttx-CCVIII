############################################################################
# apps/examples/raw_client/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

# Raw UDP socket client example

# Archivo fuente principal que contiene <progname>_main
MAINSRC = raw_udp_client.c

# Nombre del programa (coincide con la función main: raw_client_main)
PROGNAME = raw_client
PRIORITY = SCHED_PRIORITY_DEFAULT

# Ajusta esto si necesitas más stack, o define CONFIG_EXAMPLES_RAWCLIENT_STACKSIZE en Kconfig
STACKSIZE = $(CONFIG_EXAMPLES_RAWCLIENT_STACKSIZE)

# Esto es para que el ejemplo se compile solo si se activa en Kconfig
MODULE = $(CONFIG_EXAMPLES_RAWCLIENT)

include $(APPDIR)/Application.mk