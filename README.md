# SSM_VirtualMachine
 The Simple Stack Machine (SSM) is a virtual machine (VM) that is a hybrid of Montagne’s VM/0 and the MIPS processor’s ISA. In particular, the SSM is a word-addressed stack machine,  with 32-bit (4-byte) words. 

 VMOperations
 The VM has two basic modes of operation: execution of a program and (just) printing it.
 The VM is passed a single file name on its command line as an argument; it may also be passed the
 option-p as a command line argument. The file name given to the VM must be the name of a (readable)
 binary object file containing a program.
 When given the -p option, followed by a binary object file name, the VM loads the binary object file
 and prints the assembly language form of that program, see Section 2.3 for details on the assembly language.
 
 Programs are given to the VM in binary object files, which are files that contain the binary form of a program.

 Complete documentation, including usage, compilation, memory usage and operations can be found inside the provided documents:
 ssm-asm.pdf
 ssm-vm.pdf
