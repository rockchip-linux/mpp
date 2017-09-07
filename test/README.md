# Unit test description

## There are some unit test for testing mpp functions in this catalog.

### mpi_enc_test:
use sync interface(poll,dequeue and enqueue), encode raw yuv to compress video.

### mpi_dec_test:
use sync interface and async interface(decode_put_packet and decode_get_frame),
decode compress video to raw yuv.

### mpi_rc_test:
encode use detailed bitrate control config.

### mpi_rc2_test:
encode use detailed bitrate control config,and cfg param come from mpi_rc.cfg.

### mpi_test:
simple description of mpi calling method, just for reference

### mpp_event_trigger:
event trigger test.

### mpp_parse_cfg:
mpp parser cfg test.

### vpu_api_test
encode or decode use legacy interface, in order to compatible with the previous
vpu interface.
