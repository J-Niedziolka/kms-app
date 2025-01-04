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

static struct pipeline_dev modeset_device;

void drmGatherFiledes() {
	printf("todo handle it properly\n");
}

void drmGatherConnectors(int drm_fd /*todo verify argument*/, drmModeRes resources, drmModeConnector *connector_array){
	drmModeConnector *connector = NULL;
	//drmModeConnector connectors[resources.count_connectors]; //todo verify if correct array size

	for (int i = 0; i < resources.count_connectors; i++){
		connector = drmModeGetConnector(drm_fd, resources.connectors[i]);
		if (connector->connection == DRM_MODE_CONNECTED) {
			connector_array[i] = *connector;
			printf("handled connector: %u\n", connector_array[i].connector_id); /*todo verify parameter correctness*/
			//break;
		}
		else {
			//todo funcja printująca status connectora - przy listowaniu unhandled connectors można wskazać czemu jest unhandled
			printf("unhandled connector: %u\n", connector->connector_id);
			connector = NULL;
			drmModeFreeConnector(connector);
		}
	}
}

void userChooseConnector(drmModeConnector * handled_connector_array, struct pipeline_dev *user_dev){
	uint32_t connId = 0;
	/*uint32_t * conn_ptr = &connId;
	do{
		printf("Choose connector to be used: ");
		scanf("%u", &connId);
		printf("conn pointer value: %u\n", *conn_ptr);
		user_dev->connector = connId;
		printf("user_dev connector value: %u\n", user_dev->connector);
	} while (conn_ptr == NULL); */

	//printf("handled_connector_array.connector_id : %d", hauseusendled_connector_array.
	printf("dostępne connectory check\n");
	int array_lenght = sizeof(handled_connector_array)/sizeof(handled_connector_array[0]);
	for(int i= 0; i < array_lenght; i++) {
		printf("dostępne connectory, handled_connector_array[%d]: %u\n", i, handled_connector_array[i].connector_id);
	}
	printf("chyba nie wchodzi w petle xD");

	int success = 0;
	while (!success) {
	    printf("Choose either of handled connectors: ");
	    if (scanf("%u", &connId) == 1) {
		for(int i= 0; i < array_lenght; i++) {
			if(connId == handled_connector_array[i].connector_id) {
				success = 1;
				user_dev->connector = connId;
			}
		}
		printf("Choosen connector is not in the array of supported connectors!\n");
		success = 0;
	    } else {
		// Clear invalid input
		while (getchar() != '\n');
		printf("Invalid input, try again.\n");
	    }
	}


	printf("dupa1\n");
	user_dev->connector = connId;
	printf("2dupa\n");
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

	//drmGatherConnectors(drm_fd);
	//printf("wyszlismy z drmgatherconnectors\n");
	//printf("konektor: %u\n", &handled_connectors[0]);

	//gather connectors
	printf("wcześniejsza funkcja:\n");
	drmModeConnector *connector = NULL;
	drmModeConnector available_connectors_array [resources->count_connectors];
	printf("available connectors array[0] and [1]: %u, %u", available_connectors_array[0].connector_id, available_connectors_array[1].connector_id);
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

	printf("\nNowa funkcja:\n");
	drmGatherConnectors(drm_fd, *resources, available_connectors_array);
	userChooseConnector(available_connectors_array, &modeset_device);

	if(!connector){
		fprintf(stderr, "Connected connector not found, stopping.");
		drmModeFreeResources(resources);
		close(drm_fd);
		return -1;
	}


	drmModeModeInfo mode = connector->modes[0];
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
	//struct drm_mode_create_dumb create_req = { 0 };
	/*create_req.width = chosen_width;
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
		&connector_id, 1, &saved_crtc->mode);*/

	// And free everything: drmModeFreeCrtc(saved_crtc);, drmModeRmFB(fd, fb_id);, munmap(fb_ptr, create_req.size), etc.
}
