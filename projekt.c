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
	drmModeCrtc* saved_crtc;
};

static struct pipeline_dev *modeset_devices = 0;

void drmGatherFiledes() {
	printf("todo handle it properly\n");
}

void drmGatherConnectors(int drm_fd){
	/*drmModeConnector* connector = NULL;
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
	printf("wychodzimy z drmgatherconnectors\n");*/
	// Pseudocode-ish

	printf("\n--- Connectors ---\n");
	drmModeRes* res = drmModeGetResources(fd);
	for (int i = 0; i < res->count_connectors; i++) {
		drmModeConnector* conn = drmModeGetConnector(fd, res->connectors[i]);
		if (!conn) continue;

		printf("[%2d] Connector %u: ", i, conn->connector_id);
		switch (conn->connector_type) {
		case DRM_MODE_CONNECTOR_HDMIA: printf("HDMI-A"); break;
		case DRM_MODE_CONNECTOR_DisplayPort: printf("DP"); break;
		case DRM_MODE_CONNECTOR_VIRTUAL: printf("Virtual"); break;
			// etc. for other connector types
		default: printf("Unknown");
		}

		if (conn->connection == DRM_MODE_CONNECTED) {
			printf(" (connected)\n");
		}
		else {
			printf(" (not connected)\n");
		}

		// Print the modes
		for (int m = 0; m < conn->count_modes; m++) {
			drmModeModeInfo* mode = &conn->modes[m];
			printf("\tMode %2d: %dx%d @ %dHz, name=%s\n",
				m, mode->hdisplay, mode->vdisplay, mode->vrefresh, mode->name);
		}
		drmModeFreeConnector(conn);
	}
	drmModeFreeResources(res);
}

void userChooseConnector(struct pipeline_dev user_dev){
	/*uint32_t connId = 0;
	uint32_t * connPtr = &connId;
	do{
		scanf("Choose connector to be used: %u", &connId);
	} while (connPtr==NULL);

	user_dev.connector = connId;*/
	uint32_t connId = 0;
	uint32_t* connPtr = &connId;
	do {
		printf("Enter the index of a connector to use: ");
		scanf("%d", &chosen_connector_index);
		
	} while (connPtr == NULL);

	user_dev.connector = connId
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

	drmGatherConnectors(drm_fd);
	//printf("konektor: %u\n", &handled_connectors[0]);

	//gather connectors
	/*drmModeConnector* connector = NULL;
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
	}*/

	userChooseConnector();
	if(!connector){
		fprintf(stderr, "Connected connector not found, stopping.");
		drmModeFreeResources(resources);
		close(drm_fd);
		return -1;
	}

	/*drmModeModeInfo mode = connector->modes[0];
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
	return 0;*/

	// Create a dumb buffer: 
	struct drm_mode_create_dumb create_req = { 0 };
	create_req.width = chosen_width;
	create_req.height = chosen_height;
	create_req.bpp = 32;
	if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_req) < 0) {
		perror("drmIoctl(DRM_IOCTL_MODE_CREATE_DUMB) failed");
		// handle error
	}


	// Map that buffer so you can write to it:
	struct drm_mode_map_dumb map_req = { 0 };
	map_req.handle = create_req.handle;
	if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req) < 0) {
		// handle error
	}
	uint8_t* fb_ptr = mmap(0, create_req.size, PROT_READ | PROT_WRITE, MAP_SHARED,
		fd, map_req.offset);
	if (fb_ptr == MAP_FAILED) {
		// handle error
	}
	memset(fb_ptr, 0, create_req.size); // e.g. fill black

	// Add FB (using the handle from the dumb buffer):
	uint32_t fb_id;
	if (drmModeAddFB(fd, create_req.width, create_req.height,
		24, 32, create_req.pitch,
		create_req.handle, &fb_id)) {
		perror("drmModeAddFB failed");
	}

	// Set the CRTC (this actually lights it up):
	if (drmModeSetCrtc(fd, crtc_id, fb_id, 0, 0,
		&connector_id, 1, &chosen_mode)) {
		perror("drmModeSetCrtc failed");
	}

	//(Optional) Wait for user input or do something interesting with events.

	//Restore the old CRTC(if you want to restore the previous mode) or just exit.
	// before exiting
	drmModeSetCrtc(fd, saved_crtc->crtc_id, saved_crtc->buffer_id,
		saved_crtc->x, saved_crtc->y,
		&connector_id, 1, &saved_crtc->mode);

	// And free everything: drmModeFreeCrtc(saved_crtc);, drmModeRmFB(fd, fb_id);, munmap(fb_ptr, create_req.size), etc.
}
