##############################################################################
# apps/examples/my_client/CMakeLists.txt
##############################################################################
if(CONFIG_EXAMPLES_MY_CLIENT)
  nuttx_add_application(
    # Nombre del programa ejecutable en la NSH
    NAME
    my_client
    # Tamaño de la pila del programa, usando la variable de Kconfig
    STACKSIZE
    ${CONFIG_EXAMPLES_MY_CLIENT_STACKSIZE}
    # Variable de Kconfig para el módulo 
    MODULE
    ${CONFIG_EXAMPLES_MY_CLIENT}
    # Archivo fuente que contiene la función principal (main o my_client_main)
    SRCS
    my_client_main.c)
endif()
