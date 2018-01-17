NOTE: Google original ndk libc.so is lack of some symbol and it will cause error
as follows:

undefined reference to `__system_property_get'

Please use the libc.so in this directory to fix this issue.

Backup original libc.so in ndk first. Then Put lib.so to:
path_to_ndk/platforms/android-21/arch-arm64/usr/lib/
