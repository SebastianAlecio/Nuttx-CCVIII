##############################################################################
# apps/examples/my_client_raw/CMakeLists.txt
##############################################################################

if(CONFIG_EXAMPLES_MY_CLIENT_RAW)
  nuttx_add_application(
    # Nombre del programa 
    NAME
    my_client_raw
    STACKSIZE
    ${CONFIG_EXAMPLES_MY_CLIENT_RAW_STACKSIZE} 
    MODULE
    ${CONFIG_EXAMPLES_MY_CLIENT_RAW}
    SRCS
    my_client_raw_main.c)
endif()

