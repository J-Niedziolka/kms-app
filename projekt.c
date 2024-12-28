#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <string.h>

struct pipeline_dev {
	struct pipeline_dev *next;

	uint32_t width;
	uint32_t heigtht;
	uint32_t size;
	uint32_t handle;
	uint8_t *map;

	drmModeModeInfo mode;
	uint32_t connector;
	uint32_t crtc;
	uint32_t framebuffer;
	drmModeCrtc *saved_crtc;
};

static struct pipeline_dev *modeset_devices = 0;

void drmGatherFiledes() {
	printf("todo handle it properly\n");
}

void drmGatherConnectors(int drm_fd, drmModeRes * resources){
	drmModeConnector *connector = NULL;
	uint32_t temp_connectors[resources->count_connectors];
	int conns = 0;
	for (int i = 0; i < resources->count_connectors; i++) {
		connector = drmModeGetConnector(drm_fd, resources->connectors[i]);
		if (connector->connection == DRM_MODE_CONNECTED) {
			printf("handled connector: %u\n", connector->connector_id);
			temp_connectors[i] = connector->connector_id;
			//break;
			drmModeFreeConnector(connector);
			conns++;
		}
		else {
			printf("unhandled connector: %u\n", connector->connector_id);
			drmModeFreeConnector(connector);
			connector = NULL;
		}
	}

	uint32_t handled_connectors[conns];
	for(int i = 0; i < conns; i++){
		handled_connectors[i] = temp_connectors[i];
	}
	printf("wychodzimy z drmgatherconnectors\n");
}

void userChooseConnector(struct pipeline_dev user_dev){
	uint32_t connId = 0;
	uint32_t * connPtr = &connId;
	do{
		scanf("Choose connector to be used: %u", &connId);
	} while (connPtr==NULL);

	user_dev.connector = connId;
}

int main(int argc, char *argv[]) {
	int drm_fd = -1;
	if (argc < 2){ //todo handle it properly
		drmGatherFiledes();
		printf("testing if we fall into contidtion\n");
		return 0;
	}
	else {
		//util_open(argv[1]);
		//drmOpen(
		drm_fd = open(argv[1], O_RDWR | O_CLOEXEC);
	}
	if (drm_fd < 0) {
		perror("Can't open DRM device");
		return -1;
	}

	drmModeRes *resources = drmModeGetResources(drm_fd);
	if (!resources) {
		perror("Can't reach DRM resources");
		close(drm_fd);
		return -1;
	}

	drmGatherConnectors(drm_fd, resources);
	//printf("konektor: %u\n", &handled_connectors[0]);

	//gather connectors
	drmModeConnector *connector = NULL;
	for (int i = 0; i < resources->count_connectors; i++) {
		connector = drmModeGetConnector(drm_fd, resources->connectors[i]);
		if (connector->connection == DRM_MODE_CONNECTED) {
			printf("handled connector: %u\n", connector->connector_id);
			//break;
		}
		else {
			printf("unhandled connector: %u\n", connector->connector_id);
			drmModeFreeConnector(connector);
			connector = NULL;
		}
	}

	if(!connector){
		fprintf(stderr, "Connected connector not found, stopping.");
		drmModeFreeResources(resources);
		close(drm_fd);
		return -1;
	}

	drmModeModeInfo mode = connector->modes[0];
 drmModeCrtc *crtc = drmModeGetCrtc(drm_fd, resources->crtcs[0]);

    // create framebuffer
    uint32_t fb;
    drmModeAddFB(drm_fd, mode.hdisplay, mode.vdisplay, 24, 32, mode.hdisplay * 4, crtc->buffer_id, &fb);


    // Wyświetl pusty (czarny) ekran przez kilka sekund
    sleep(5);

    // Posprzątaj
    drmModeRmFB(drm_fd, fb);
    drmModeFreeCrtc(crtc);
    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);
    close(drm_fd);
	return 0;
}
