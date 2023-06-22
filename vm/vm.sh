make clean
make
cd build/
clear

# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/fork-read:fork-read -p ../../tests/userprog/sample.txt:sample.txt --swap-disk=4 -- -q   -f run fork-read
# make tests/userprog/fork-read.result



## check_address
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/pt-write-code2:pt-write-code2 -p ../../tests/vm/sample.txt:sample.txt --swap-disk=4 -- -q   -f run pt-write-code2
# make tests/vm/pt-write-code2.result
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/pt-grow-stk-sc:pt-grow-stk-sc --swap-disk=4 -- -q   -f run pt-grow-stk-sc
# make tests/vm/pt-grow-stk-sc.result


# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/mmap-read:mmap-read -p ../../tests/vm/sample.txt:sample.txt --swap-disk=4 -- -q   -f run mmap-read
# make tests/vm/mmap-read.result

# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/mmap-close:mmap-close -p ../../tests/vm/sample.txt:sample.txt --swap-disk=4 -- -q   -f run mmap-close
# make tests/vm/mmap-close.result

# pintos -v -k -T 180 -m 8   --fs-disk=10 -p tests/vm/swap-file:swap-file -p ../../tests/vm/large.txt:large.txt --swap-disk=10 -- -q   -f run swap-file

# # make tests/vm/mmap-off.result
# # make tests/vm/mmap-remove.result
# # make tests/vm/mmap-close.result
# make tests/vm/page-merge-par.result
# make tests/vm/page-merge-stk.result
# make tests/vm/page-merge-mm.result
pintos -v -k -T 120 -m 20   --fs-disk=10 -p tests/vm/pt-grow-stk-sc:pt-grow-stk-sc --swap-disk=4 -- -q   -f run pt-grow-stk-sc
make tests/vm/pt-grow-stk-sc.result

# pintos -v -k -T 180 -m 8   --fs-disk=10 -p tests/vm/swap-file:swap-file -p ../../tests/vm/large.txt:large.txt --swap-disk=10 -- -q   -f run swap-file
# make tests/vm/swap-file.result
# pintos -v -k -T 180 -m 10   --fs-disk=10 -p tests/vm/swap-anon:swap-anon --swap-disk=30 -- -q   -f run swap-anon
# make tests/vm/swap-anon.result
# pintos -v -k -T 180 -m 10   --fs-disk=10 -p tests/vm/swap-iter:swap-iter -p ../../tests/vm/large.txt:large.txt --swap-disk=50 -- -q   -f run swap-iter
# make tests/vm/swap-iter.result


# make tests/vm/page-merge-par.result
# make tests/vm/page-merge-stk.result
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/page-merge-mm:page-merge-mm -p tests/vm/child-qsort-mm:child-qsort-mm --swap-disk=10 -- -q   -f run page-merge-mm
# make tests/vm/page-merge-mm.result
# pintos -v -k -T 180 -m 10   --fs-disk=10 -p tests/vm/swap-anon:swap-anon --swap-disk=30 -- -q   -f run swap-anon
# make tests/vm/swap-anon.result

# make tests/vm/page-merge-mm.result
# make tests/vm/mmap-exit.result
# make tests/vm/mmap-inherit.result

cd ..