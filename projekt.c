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

static struct modeset_dev *modeset_list = 0;

void drmGatherConnectors(int drm_fd /*todo verify argument*/, drmModeRes resources){
	drmModeConnector *connector = NULL;
	drmModeConnector connectors[resources.count_connectors]; //todo verify if correct array size

	for (int i = 0; i < resources.count_connectors; i++){
		connector = drmModeGetConnector(drm_fd, resources.connectors[i]);
		if (connector->connection == DRM_MODE_CONNECTED) {
			connectors[i] = *connector;
			printf("handled connector: %lu\n", connector->connector_id);
			//break;
		}
		else {
			//todo funcja printująca status connectora - przy listowaniu unhandled connectors można wskazać czemu jest unhandled
			printf("unhandled connector: %lu\n", connector->connector_id);
			drmModeFreeConnector(connector);
			connector = NULL;
		}
	}
}

void userChooseConnector(struct pipeline_dev user_dev){
	uint32_t connId = 0;
	do{
		scanf("Choose connector to be used: %d", &connId);
	} while (connId ==NULL);

	user_dev.connector = connId;
}

int main(int argc, char *argv[]) {
	int drm_fd = open("/dev/dri/card1", O_RDWR | O_CLOEXEC);
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

	//gather connectors
	printf("wcześniejsza funkcja:\n");
	drmModeConnector *connector = NULL;
	for (int i = 0; i < resources->count_connectors; i++) {
		connector = drmModeGetConnector(drm_fd, resources->connectors[i]);
		if (connector->connection == DRM_MODE_CONNECTED) {
			printf("handled connector: %lu\n", connector->connector_id);
			//break;
		}
		else {
			printf("unhandled connector: %lu\n", connector->connector_id);
			drmModeFreeConnector(connector);
			connector = NULL;
		}
	}

	printf("\nNowa funkcja:\n");
	drmGatherConnectors(drm_fd, *resources);
	userChooseConnector(modeset_list);

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
