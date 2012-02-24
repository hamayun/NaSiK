##===- projects/c6x-decoder/tools/c6x-decoder/Makefile ---------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of the tool.
#
TOOLNAME=c6x-decoder

#
# List libraries that we'll need
# We use LIBS because sample is a dynamic library.
#
USEDLIBS = BinaryReader.a InstructionMultilister.a InstructionDecoder.a LLVMGenerator.a

LINK_COMPONENTS := bitwriter bitreader nativecodegen ipo
CXXFLAGS += -fno-strict-aliasing -g -D${C6XISA_VERSION}

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

