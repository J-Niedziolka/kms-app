#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <string.h>
#include <drm_fourcc.h>
#include <sys/mman.h>
#include <errno.h>

#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_RESET "\x1b[0m"

struct pipeline_dev {
	struct pipeline_dev *next;

	uint32_t width;
	uint32_t heigtht;
	uint32_t size;
	uint32_t handle;
	uint8_t *map;

	//drmModeModeInfo mode; //todo potencjalnie do przerobienia na drmModeModeInfoPtr, będzie łatwiej przechowywać wskaźnik do modu niż te wartości powyżej
	drmModeModeInfoPtr modePtr;
	uint32_t connector; //todo czy to nie problem, że przechowuję same uinty - a więc IDs devów zamiast wskaźników do obiektów?
	uint32_t crtc;
	uint32_t framebuffer;
	uint32_t encoder_id; // todo np "drmModeGetEncoder(drm_fd, encoder_id)" zamiast user_dev.encoder = choosen_encoder_id
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
					user_dev->connector = connId; //todo współczesne podejście zakłada raczej użycie drmModeAttachMode
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
void drmListAvailableModes(drmModeConnector chosenConnector){
	int availableModeNumber = chosenConnector.count_modes;
	drmModeModeInfo modesArray [availableModeNumber];

	for(int i = 0; i < availableModeNumber; i++)
		modesArray[i] = chosenConnector.modes[i];

	for(int i = 0; i < availableModeNumber; i++){ //todo print nicely formatted output
		printf("modesArray Mode[%d] info: name: %s, clock: %u\t", i, modesArray[i].name, modesArray[i].clock);
		printf("display height (Vert.): %u, display width (Hor.): %u\n", modesArray[i].hdisplay, modesArray[i].vdisplay);
	}
}

int userChooseDrmMode(drmModeConnector chosenConnector, struct pipeline_dev *user_dev, int runtimeMode){
	int userChooseModeIndex = -1;
	if(!runtimeMode){
		//user_dev->mode = chosenConnector.modes[0];
		userChooseModeIndex = 0;
	}
	else {
		printf("Do you want to dump mode info? y/N : ");
		char select = 'n';
		scanf(" %c", &select);
		if(select == 'y' || select == 'Y')
			drmListAvailableModes(chosenConnector);

		printf("Choose DRM Mode: ");
		scanf("%d", &userChooseModeIndex);
		//user_dev->mode = chosenConnector.modes[userChooseModeIndex]; //todo dodać obsługę błędów przy wprowadzeniu złego Mode
	}

	//printf("drmMode height and width: %u x %u\n", user_dev->mode.hdisplay, user_dev->mode.vdisplay);
	if(userChooseModeIndex >= 0 && userChooseModeIndex < chosenConnector.count_modes) {
		user_dev->modePtr = &chosenConnector.modes[userChooseModeIndex];
		printf("Confirming Mode %s\n", user_dev->modePtr->name);
		return userChooseModeIndex;
	}
	else {
		user_dev->modePtr = &chosenConnector.modes[0];
		printf("Not a correct mode chosen! Defaulting to Mode#0: %s\n", user_dev->modePtr->name);
		return 0;
	}
	//return userChooseModeIndex;
}

void drmListAvailableEncoders(int drm_fd, drmModeConnector chosenConnector){
	int availableEncoderNumber = chosenConnector.count_encoders;
	drmModeEncoder * encodersArray [availableEncoderNumber];

	/*for(int i = 0; i < availableEncoderNumber; i++){
		//encodersArray[i] = drmModeGetEncoder(drm_fd, chosenConnector.encoders[i]);
	}*/

	for(int i = 0; i < availableEncoderNumber; i++){ //todo print nicely formatted output
		encodersArray[i] = drmModeGetEncoder(drm_fd, chosenConnector.encoders[i]);
		printf("Encoder#" ANSI_GREEN "%u" ANSI_RESET "info: \t", encodersArray[i]->encoder_id);
		printf("encoder type: %u, crtc id: %u\n", encodersArray[i]->encoder_type, encodersArray[i]->crtc_id);
	}
}

int userChooseDrmEncoder(int drm_fd, drmModeConnector chosenConnector, struct pipeline_dev *user_dev, int runtimeMode){
	int userChooseEncoderId = -1;
	if(!runtimeMode){
		// user_dev->encoder_id = chosenConnector.encoders[0]; // user_dev->encoder = id konektora z encoders[0], np 105
		userChooseEncoderId = chosenConnector.encoders[0]; // userChooseEncoderIndex = jak wyżej	 //todo linię wyżej przypisuję wartość do zmiennej globalnej user_dev->encoder; nie wiem, czy konieczne jest handlowanie dodatkowej zmiennej userChooseEncoderIndex
		//todo czym właściwie jest choosenConnector.encoders[0]? co dokładnie przypisuję do userChooseEncoderIndex? czy nie powinienem zmienić nazwy zmiennej? ew. przypisać userChooseEncoderIndex = 0;
		user_dev->encoder_id = userChooseEncoderId;
	}
	else {
		printf("Do you want to dump encoder info? y/N : ");
		char select = 'n';
		scanf(" %c", &select);
		if(select == 'y' || select == 'Y')
			drmListAvailableEncoders(drm_fd, chosenConnector);

		printf("Choose DRM Encoder ID: ");
		scanf("%d", &userChooseEncoderId);
		//user_dev->encoder_id = chosenConnector.encoders[userChooseEncoderId]; //todo dodać obsługę błędów przy wprowadzeniu złego Encodera:
		if(!drmModeGetEncoder(drm_fd, userChooseEncoderId)) {
			printf("Wrong encoder ID! It has been replaced with a default one.\n");
			userChooseEncoderId = chosenConnector.encoders[0];
		}
		user_dev->encoder_id = userChooseEncoderId;
	}

	printf("Chosen drmEncoder ID: %u\n", user_dev->encoder_id);
	return userChooseEncoderId;
}

void drmListAvailableCrtcs(drmModeRes *resources, int drm_fd, drmModeEncoder *enc)
{
	printf("Available CRTCs for encoder %u:\n", enc->encoder_id);

	for (int i = 0; i < resources->count_crtcs; i++) {
		if (enc->possible_crtcs & (1 << i)) { //todo to be refactored; wynik operacji bitwise nie powinien być traktowany jako wartość boolean
			uint32_t crtc_id = resources->crtcs[i];
			drmModeCrtc *crtc = drmModeGetCrtc(drm_fd, crtc_id);
			if (!crtc) {
				printf("Can't fetch info about CRTC#%u", crtc->crtc_id);
				continue;
			}
			printf("  [%d] CRTC ID: " ANSI_GREEN "%u" ANSI_RESET ", mode name: %s\n", i, crtc->crtc_id, crtc->mode.name); //todo bug: w kroku userChooseDrmMode wybieram mod np 1920x1080, a crtc->mode.name pokazuje np 3840x2160
			// aby rozwiązać tego buga, prawdopodobnie muszę ręcznie "odczepić" defaultowy mode od crtc i przypisać ten którego wybrał user
			//update: to crtc wskazuje aktualnie użyte crtc
			drmModeFreeCrtc(crtc);
		}
	}
}

int userChooseDrmCrtcs(drmModeRes *resources, int drm_fd, drmModeEncoder *enc, struct pipeline_dev *user_dev, int runtimeMode)
{
	int crtc_index = -1;

	if (!runtimeMode) {
		for (int i = 0; i < resources->count_crtcs; i++) {
			if (enc->possible_crtcs & (1 << i)) {
				crtc_index = i;
				break;
			}
		}
	} else {
		printf("Do you want to dump CRTC info? Y/n : ");
		char select = 'n';
		scanf(" %c", &select);
		if (select == 'y' || select == 'Y') {
			drmListAvailableCrtcs(resources, drm_fd, enc);
		}
		else if (select == 'n' || select == 'N'){

		}
		else {
			drmListAvailableCrtcs(resources, drm_fd, enc);
		}

		printf("Choose CRTC index (indeks tablicy): ");
		if (scanf("%d", &crtc_index) != 1) {
			while (getchar() != '\n');
			fprintf(stderr, "Invalid input for CRTC index.\n");
			return -1;
		}
	}

	if (crtc_index < 0 || crtc_index >= resources->count_crtcs) {
		fprintf(stderr, "No valid CRTC index found for encoder %u.\n", enc->encoder_id);
		return -1;
	}

	// check that this chosen index is actually in the bitmask
	if (!(enc->possible_crtcs & (1 << crtc_index))) {
		fprintf(stderr, "CRTC index %d is not possible for encoder %u.\n",
			crtc_index, enc->encoder_id);
		return -1;
	}

	uint32_t chosen_crtc_id = resources->crtcs[crtc_index];
	drmModeCrtc *crtc = drmModeGetCrtc(drm_fd, chosen_crtc_id);
	if (!crtc) {
	fprintf(stderr, "drmModeGetCrtc() failed for CRTC index %d.\n",
		crtc_index);
	return -1;
	}

	user_dev->crtc = crtc->crtc_id;
	user_dev->saved_crtc = crtc; //todo saved_crtc musi być freed() potem
	//todo różnica między saved_crtc, crtc_clone, possible_crtc; czym jest encoders.possible_crtcs


	drmModeFreeCrtc(crtc);

	return crtc_index;
}

int createDumbFB2(int drm_fd, struct pipeline_dev *user_dev){
	struct drm_mode_create_dumb creq = {0};
	creq.width = user_dev->modePtr->hdisplay;
	creq.height = user_dev->modePtr->vdisplay;
	creq.bpp = 32;

	if (drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) != 0) {
		perror("DRM_IOCTL_MODE_CREATE_DUMB failed");
		return -errno;
	}

	user_dev->width = creq.width;
	user_dev->heigtht = creq.height;
	user_dev->size = creq.size;
	user_dev->handle = creq.handle;

	struct drm_mode_map_dumb mreq = {0};
	mreq.handle = user_dev->handle;
	if (drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) != 0){
		perror("DRM_IOCTL_MODE_MAP_DUMB failed");
		return -errno;
	}

	user_dev->map = mmap(NULL, user_dev->size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mreq.offset);
	if (user_dev->map == MAP_FAILED) {
		perror("mmap failed");
		return -errno;
	}

	memset(user_dev->map, 0x00, user_dev->size);

	uint32_t bo_handles[4] = { user_dev->handle, 0, 0, 0 };
	uint32_t pitches[4]    = { creq.pitch, 0, 0, 0 };
	uint32_t offsets[4]    = { 0,         0, 0, 0 };
	uint32_t format = DRM_FORMAT_XRGB8888;

	int ret = drmModeAddFB2(drm_fd, user_dev->width, user_dev->heigtht, format, bo_handles, pitches, offsets, &user_dev->framebuffer, 0);

	if (ret) {
		perror("drmModeAddFB2 failed");
		munmap(user_dev->map, user_dev->size);
		user_dev->map = NULL;
		return -errno;
	}

	printf("Created FB %u (%ux%u)\n", user_dev->framebuffer, user_dev->width, user_dev->heigtht);
	return 0;
}

static inline uint32_t make_xrgb8888(uint8_t r, uint8_t g, uint8_t b)
{
    /* XRGB8888: 8 bits each for R, G, B, plus an unused alpha channel. */
    return (0xff << 24) | (r << 16) | (g << 8) | (b << 0);
}

void fill_stripes_xrgb8888(uint8_t *fb_ptr,
                           int width,
                           int height,
                           int stride)
{
    /* Define some colors for stripes: black, red, green, blue, yellow, magenta */
    uint32_t colors[] = {
        make_xrgb8888(0x00, 0x00, 0x00), // black
        make_xrgb8888(0xff, 0x00, 0x00), // red
        make_xrgb8888(0x00, 0xff, 0x00), // green
        make_xrgb8888(0x00, 0x00, 0xff), // blue
        make_xrgb8888(0xff, 0xff, 0x00), // yellow
        make_xrgb8888(0xff, 0x00, 0xff), // magenta
    };
    int num_colors = sizeof(colors) / sizeof(colors[0]);

    /* Each stripe will be width/num_colors wide. */
    for (int y = 0; y < height; y++) {
        /* row_ptr: pointer to the start of row y */
        uint32_t *row_ptr = (uint32_t *)(fb_ptr + y * stride);
        for (int x = 0; x < width; x++) {
            int stripe_idx = (x * num_colors) / width; // which stripe are we in?
            row_ptr[x] = colors[stripe_idx];
        }
    }
}

int main(int argc, char *argv[]) {
	printf("%ld",__STDC_VERSION__);
	int runtimeMode = 1; //todo dodać dynamiczny wybór runtimeMode: manual, automatyczny, verbose (printuje najpierw całość, potem ktoś sobie wybiera)
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
	drmModeConnector *available_connectors_array [resources->count_connectors]; //todo array[count_connectors] jest przekazany do drmGatherConnectors i wraca wypełniony tylko do array[filled]; warto byłoby uciąć niepotrzebną końcówkę

	int connected_count = drmGatherConnectors(drm_fd, resources, available_connectors_array); //todo nie wiem czy jest potrzeba bawienia się z przekazywaniem tutaj jakiejś tablicy + z tym array powyżej, czy coś z tym w ogóle robię? sprawdzic

	int userChosenConnectorIndex = userChooseConnector(available_connectors_array, connected_count, &modeset_device);
	if (userChosenConnectorIndex < 0) {
		//todo handle this case
	}
	connector = available_connectors_array[userChosenConnectorIndex]; // todo zamiast 'connector' możnaby używać user_dev->connector; sprawdzić

	if(!connector){
		fprintf(stderr, "Connected connector not found, stopping.");
		drmModeFreeResources(resources);
		close(drm_fd);
		return -1;
	}

	int userChosenModeIndex = userChooseDrmMode(*connector, &modeset_device, runtimeMode); //todo wydaje mi się, że tu wystarczyłaby sama wartość connectora - czy *connector jest konieczne?
	//if(userChosenModeIndex >= 0 && userChosenModeIndex < connector->count_modes)
	//	modeset_device.modePtr = &connector->modes[userChosenModeIndex];
	int userChosenEncoderIndex = userChooseDrmEncoder(drm_fd, *connector, &modeset_device, runtimeMode);
	drmModeEncoder *enc = drmModeGetEncoder(drm_fd, modeset_device.encoder_id);
	int userChosenCrtcIndex = userChooseDrmCrtcs(resources, drm_fd, enc, &modeset_device, runtimeMode);

	printf("choosen crtc id: %d", userChosenCrtcIndex);

	int err = createDumbFB2(drm_fd, &modeset_device);
	if (err != 0) {
		fprintf(stderr, "createDumbFB2 failed: %d\n", err);
		//todo obsługa błędów, zwolnienie zasobów
	}

	int ret = drmModeSetCrtc(drm_fd, modeset_device.crtc, modeset_device.framebuffer, 0, 0, &modeset_device.connector, 1, modeset_device.modePtr);

	while (1) {
		pause();
	}

	if (ret) {
		perror("drmModeSetCRTC fail!!");
		//todo obsługa
	}

drmModeSetCrtc(drm_fd,
		modeset_device.saved_crtc->crtc_id,
		modeset_device.saved_crtc->buffer_id,
		modeset_device.saved_crtc->x,
		modeset_device.saved_crtc->y,
		&modeset_device.connector,
		1,
		&modeset_device.saved_crtc->mode);
	drmModeFreeCrtc(modeset_device.saved_crtc);

	drmModeRmFB(drm_fd, modeset_device.framebuffer);
	munmap(modeset_device.map, modeset_device.size);
	modeset_device.map = NULL;

	struct drm_mode_destroy_dumb dreq = {0};
	dreq.handle = modeset_device.handle;
	drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);

 /*drmModeCrtc *crtc = drmModeGetCrtc(drm_fd, resources->crtcs[0]);

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
