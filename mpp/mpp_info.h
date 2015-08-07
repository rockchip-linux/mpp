#ifndef _mpp_VPU_VERSION_
#define _mpp_VPU_VERSION_

typedef enum RK_CHIP_TYPE {
    NONE,
    RK29,
    RK30,
    RK31,

    RK_CHIP_NUM = 0x100,
} RK_CHIP_TYPE; 

#ifdef __cplusplus
class mpp_info
{
public:
    RK_CHIP_TYPE get_chip_type() {return chip_type;}
    int  get_mpp_revision() {return mpp_revision;}
    void show_mpp_info();

    static mpp_info *getInstance();
    virtual ~mpp_info() {};
private:
    static mpp_info *singleton;
    mpp_info();
    int      mpp_revision;
    char    *mpp_info_revision;
    char    *mpp_info_author;
    char    *mpp_info_date;
    RK_CHIP_TYPE chip_type;
};

extern "C" {
#endif /* __cplusplus */
RK_CHIP_TYPE get_chip_type();
#ifdef __cplusplus
}
#endif

#endif
