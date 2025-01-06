#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/select.h>
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

int drmGatherConnectors(int drm_fd, drmModeRes * resources, drmModeConnector **connector_array){
	int filled = 0;

	for (int i = 0; i < resources->count_connectors; i++){
		drmModeConnector * conn = drmModeGetConnector(drm_fd, resources->connectors[i]);
		if (!conn)
			continue;

		if (conn->connection == DRM_MODE_CONNECTED) {
			connector_array[filled] = conn;
			printf("handled connector: %u\n", connector_array[filled]->connector_id); //todo verify parameter correctness
			filled++;
		}
		else {
			//todo funcja printująca status connectora - przy listowaniu unhandled connectors można wskazać czemu jest unhandled
			printf("unhandled connector: %u\n", conn->connector_id);
			drmModeFreeConnector(conn); //todo pozbylem się unhandled connectorów, gdybym chciał je dodatkowo obsłużyć
				//to muszę pozbyć się tego free() - pewnie będzie segfault albo śmieciowe wyniki
			conn = NULL;
			continue;
		}
	}
	return filled;
}

int userChooseConnector(drmModeConnector ** handled_connector_array, int lenght, struct pipeline_dev *user_dev){
	uint32_t connId = 0;
	int choosenIndex = -1;
	int success = 0;

	while (!success) {
		printf("Choose either of handled connectors: ");
		if (scanf("%u", &connId) == 1) {
			printf("wybrales - %d\n", connId);
			for(int i= 0; i < lenght; i++) {
				if(connId == handled_connector_array[i]->connector_id) {
					printf("weszlo w petle!\n");
					choosenIndex = i;
					user_dev->connector = connId;
					success = 1;
					printf("sukces!\n");
					break;
				//todo: refactor, poprawić indentację poniżej
				//} //todo tu jest bug: jeśli 4 linie poniżej są zakomentowane, to z jakiegoś powodu konektor ustawia się poprawnie, ale i tak printuje loga poniżej przy wybraniu innego niż pierwszy z dostępnych konektorów
			} else if (i == lenght-1 && connId != handled_connector_array[i]->connector_id) {
				printf("Choosen connector is not in the array of supported connectors!\n");
			} else {
				continue;
			}
			if (!success) {
				printf("Choosen connector is not in the array of supported connectors!\n");
			}
		}
	} else {
		// clear invalid input
		while (getchar() != '\n');
		printf("Invalid input, try again.\n");
	    }
	}
	return choosenIndex;
}

/*typedef struct _drmModeModeInfo {
uint32_t clock;
	uint16_t hdisplay, hsync_start, hsync_end, htotal, hskew;
	uint16_t vdisplay, vsync_start, vsync_end, vtotal, vscan;

	uint32_t vrefresh;

	uint32_t flags;
	uint32_t type;
	char name[DRM_DISPLAY_MODE_LEN];
} drmModeModeInfo, *drmModeModeInfoPtr;

*/
void drmListAvailableModes(drmModeConnector choosenConnector){
	int availableModeNumber = choosenConnector.count_modes;
	drmModeModeInfo modesArray [availableModeNumber];

	for(int i = 0; i < availableModeNumber; i++)
		modesArray[i] = choosenConnector.modes[i];

	for(int i = 0; i < availableModeNumber; i++){
		printf("modesArray Mode[%d] info: clock: %u\t", i, modesArray[i].clock);
		printf("display height (Vert.): %u, display width (Hor.): %u", modesArray[i].hdisplay, modesArray[i].vdisplay);
	}
}

int drmGatherModes(){
	return 0;
}

void userChooseDrmMode(drmModeConnector choosenConnector){
	int
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

	//gathering connectors
	drmModeConnector *connector = NULL;
	drmModeConnector *available_connectors_array [resources->count_connectors];

	int connected_count = drmGatherConnectors(drm_fd, resources, available_connectors_array);

	int choosen_connector_index = userChooseConnector(available_connectors_array, connected_count, &modeset_device);
	if (choosen_connector_index < 0) {
		//todo handle this case
	}
	connector = available_connectors_array[choosen_connector_index];

	if(!connector){
		fprintf(stderr, "Connected connector not found, stopping.");
		drmModeFreeResources(resources);
		close(drm_fd);
		return -1;
	}


	printf("Do you want to dump mode info? y/N : ");
	char select = 'n';
	scanf(" %c", &select);
	if(select == 'y' || select == 'Y')
		drmListAvailableModes(*connector);
	userChooseDrmMode();

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
