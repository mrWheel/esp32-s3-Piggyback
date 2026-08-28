# Opening the repository root in VS Code builds the complete interactive test program.
idf_component_register(SRCS "../examples/basic/testProgram/main/main.c"
  INCLUDE_DIRS "../examples/basic/testProgram/main" REQUIRES esp32_s3_piggyback)
