#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_shm.h>

void Test_ztl_shm_readonly(ZuTest* zt)
{
    const char* filename = "test_zshm.txt";
    int rv;
    ztl_shm_t* zshm;

    zshm = ztl_shm_create(filename, ztl_open_only, ztl_read_only, false);
    if (!zshm) {
        return;
    }

    // use 0 to auto get file size internally
    rv = ztl_shm_truncate(zshm, 0);
    ZuAssertTrue(zt, rv == 0);

    rv = ztl_shm_map_region(zshm, ztl_read_only);
    ZuAssertTrue(zt, rv == 0);

    // read shared memory data
    char* lpAddr;
    lpAddr = ztl_shm_get_address(zshm);
    ZuAssertTrue(zt, lpAddr != NULL);
    ZuAssertTrue(zt, strlen(lpAddr) > 0);

    ztl_shm_release(zshm);
}

void Test_ztl_shm_readwrite(ZuTest* zt)
{
    const char* filename = "test_zshm_rw.txt";
    int rv;
    ztl_shm_t* zshm;

    zshm = ztl_shm_create(filename, ztl_open_or_create, ztl_read_write, false);
    if (!zshm) {
        return;
    }

    // use 0 to auto get file size internally
    rv = ztl_shm_truncate(zshm, 128);
    ZuAssertTrue(zt, rv == 0);

    rv = ztl_shm_map_region(zshm, ztl_read_write);
    ZuAssertTrue(zt, rv == 0);

    char* lpAddr;
    lpAddr = ztl_shm_get_address(zshm);
    ZuAssertTrue(zt, lpAddr != NULL);

    // write data to shared memory
    char lDataBuf[1024] = "hello world\r\nthis is a demo shm test";
    int  lLength = (int)strlen(lDataBuf);
    memcpy(lpAddr, lDataBuf, lLength);

    ztl_shm_release(zshm);

    // read and compare
    zshm = ztl_shm_create(filename, ztl_open_or_create, ztl_read_write, false);
    if (!zshm) {
        return;
    }

    rv = ztl_shm_truncate(zshm, 128);
    ZuAssertTrue(zt, rv == 0);

    rv = ztl_shm_map_region(zshm, ztl_read_write);
    ZuAssertTrue(zt, rv == 0);

    lpAddr = ztl_shm_get_address(zshm);
    ZuAssertTrue(zt, lpAddr != NULL);

    ZuAssertTrue(zt, strncmp(lpAddr, lDataBuf, lLength) == 0);

    ztl_shm_release(zshm);
}
