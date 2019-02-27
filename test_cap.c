#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>
#include <fitsio.h>
#include <xpa.h>
#include <sys/time.h>
#include <ASICamera2.h>

int main(int argc, char *argv[]) {
  fitsfile *fptr;
  long fpixel=1, naxes[2];
  int width, height;
  unsigned int min_bytes, max_bytes, max_width, max_height;
  uint64_t total_bytes = 0;
  struct timeval start_time, end_time;
  time_t start_sec, end_sec;
  suseconds_t start_usec, end_usec;
  float exptime, elapsed_time, fps;
  char *filename;
  int i, gain, exp_ms, fitstatus;
  int cam = 0, bin = 1, imtype = ASI_IMG_RAW8;

  unsigned char *imbuf;

  filename = argv[1];
  exptime = atof(argv[2]);
  gain = atoi(argv[3]);

  printf("%f second exposure at gain=%d being saved to %s.\n", exptime, gain, filename);

  int nCamera = ASIGetNumOfConnectedCameras();

  if (nCamera <= 0) {
    printf("No camera connected.\n");
    return -1;
  }

  printf("Attached cameras:\n");

  ASI_CAMERA_INFO ASICameraInfo;

  for (i=0; i < nCamera; i++) {
    ASIGetCameraProperty(&ASICameraInfo, i);
    printf("%d %s\n", i, ASICameraInfo.Name);
  }

  if (ASIOpenCamera(cam) != ASI_SUCCESS) {
    printf("Error opening camera %d!", cam);
    return -1;
  }
  ASIInitCamera(cam);

  // give the camera a second to gather its wits...
  usleep(1000000);

  printf("%s information:\n", ASICameraInfo.Name);
  max_width = ASICameraInfo.MaxWidth;
  max_height = ASICameraInfo.MaxHeight;
  printf("Resolution: %dx%d\n", max_width, max_height);

  ASI_CONTROL_CAPS ControlCaps;
  int nctrl = 0;
  ASIGetNumOfControls(cam, &nctrl);
  for (i=0; i < nctrl; i++) {
    ASIGetControlCaps(cam, i, &ControlCaps);
    printf("%s\n", ControlCaps.Name);
  }

  long camtemp = 0;
  ASI_BOOL bAuto = ASI_FALSE;
  ASIGetControlValue(cam, ASI_TEMPERATURE, &camtemp, &bAuto);
  printf("Sensor Temperature: %f\n", (float)camtemp/10.0);

  while(ASI_SUCCESS != ASISetROIFormat(cam, max_width, max_height, bin, imtype));
  ASIGetROIFormat(cam, &width, &height, &bin, (ASI_IMG_TYPE*)&imtype);
  naxes[0] = width;
  naxes[1] = height;

  long imsize = width * height;
  if (!(imbuf = malloc(imsize * sizeof(char)))) {
    printf("Couldn't allocate image buffer.\n");
    return -1;
  }

  fits_create_file(&fptr, filename, &fitstatus);
  fits_create_img(fptr, BYTE_IMG, 2, naxes, &fitstatus);

  ASISetControlValue(cam, ASI_GAIN, gain, ASI_FALSE);

  int exp_us = (int)exptime * 1.0e6;
  ASISetControlValue(cam, ASI_EXPOSURE, exp_us, ASI_FALSE);
  ASISetControlValue(cam, ASI_BANDWIDTHOVERLOAD, 40, ASI_FALSE);

  ASI_EXPOSURE_STATUS status;
  ASIStartExposure(cam, ASI_FALSE);
  usleep(10000);
  status = ASI_EXP_WORKING;

  while(status == ASI_EXP_WORKING) {
    ASIGetExpStatus(cam, &status);
  }
  if (status == ASI_EXP_SUCCESS) {
    ASIGetDataAfterExp(cam, imbuf, imsize);
    fits_write_img(fptr, TBYTE, fpixel, imsize, imbuf, &fitstatus);
  }
  fits_close_file(fptr, &fitstatus);
  ASIStopExposure(cam);
  ASICloseCamera(cam);
  return 1;
}
