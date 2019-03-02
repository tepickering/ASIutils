#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <argp.h>
#include <stdbool.h>
#include <sys/time.h>

#include <fitsio.h>
#include <xpa.h>

#include <ASICamera2.h>

const char *argp_program_version = "test_cap 0.0.1dev";
const char *argp_program_bug_address = "<te.pickering@gmail.com>";
static char doc[] = "Test ZWO ASI camera by taking single frame and saving it to a FITS image.";
static char args_doc[] = "";
static struct argp_option options[] = {
    {"output", 'o', "FITSFILE", 0, "Output FITS file (default: test.fits)."},
    {"exptime", 'e', "EXPTIME", 0, "Exposure time in seconds (default: 1.0)."},
    {"gain", 'g', "GAIN", 0, "Detector gain (default: 0)."},
    {"binning", 'b', "BINNING", 0, "Detector binning (default: 1)."},
    {"width", 'w', "WIDTH", 0, "ROI width (default: detector max)."},
    {"height", 'h', "HEIGHT", 0, "ROI height (default: detector max)."},
    {"x_start", 'x', "X", 0, "X-axis start position (default: centered ROI)."},
    {"y_start", 'y', "Y", 0, "Y-axis start position (default: centered ROT)."},
    { 0 }
};

struct arguments {
    char *output;
    float exptime;
    int gain;
    int binning;
    int width;
    int height;
    int x_start;
    int y_start;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
        case 'o':
            arguments->output = arg;
            break;
        case 'e':
            arguments->exptime = atof(arg);
            break;
        case 'g':
            arguments->gain = atoi(arg);
            break;
        case 'b':
            arguments->binning = atoi(arg);
            break;
        case 'w':
            arguments->width = atoi(arg);
            break;
        case 'h':
            arguments->height = atoi(arg);
            break;
        case 'x':
            arguments->x_start = atoi(arg);
            break;
        case 'y':
            arguments->y_start = atoi(arg);
            break;
        case ARGP_KEY_ARG:
            argp_usage(state);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    };
    return 0;
};

static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char **argv) {
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

    struct arguments arguments;

    arguments.output = "!test.fits";
    arguments.exptime = 1.0;
    arguments.gain = 0;
    arguments.binning = 1;
    arguments.width = 0;
    arguments.height = 0;
    arguments.x_start = 0;
    arguments.y_start = 0;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    filename = arguments.output;
    exptime = arguments.exptime;
    gain = arguments.gain;

    if (arguments.binning > 0 && arguments.binning <= 4) {
        bin = arguments.binning;
    } else {
        printf("Allowed binning values in range of 1-4. Defaulting to 1x1...\n");
    }

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

    if (arguments.width > 0 && arguments.width <= max_width) {
        width = arguments.width;
    } else {
        width = max_width;
    };
    if (arguments.height > 0 && arguments.height <= max_height) {
        height = arguments.height;
    } else {
        height = max_height;
    };

    while(ASI_SUCCESS != ASISetROIFormat(cam, width, height, bin, imtype));
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
