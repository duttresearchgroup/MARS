#!/usr/bin/python

size = 16384

print("const int vitamins_bm_bigswitch_size = %d;"%(size))
print("void vitamins_bm_bigswitch(int n, volatile int *out) {")
print("    switch(n) {")
for i in range(size):
    print("        case %d: *out += n; break;"%(i))
print("    }")
print("    *out += *out;")
print("}\n")
    
