set(LLVM_LINK_COMPONENTS bitwriter)
set(LLVM_REQUIRES_EH 1)

add_llvm_tool(tmsc6x-dis
  tmsc6x-dis.cpp
  block_packets.cpp
  coff_reader.cpp
  tms320C6x_instruction.cpp
  transcode_instruction.cpp
  )
