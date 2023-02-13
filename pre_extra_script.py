Import("env")

# access to global construction environment
print(env.Dump())

print("#"*20)


# Dump construction environment (for debug purpose)
#print(env.Dump())

# append extra flags to global build environment
# which later will be used to build:
# - project source code
# - frameworks
# - dependent libraries

env.ProcessUnFlags("-DVER=10")
