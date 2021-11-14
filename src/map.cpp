#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include "utils.h"
#include "log.h"
#include "evcurl.h"
#include "map.h"


#define TILESIZE 256
#define MAX_ZOOM 20
#define MIN_ZOOM 0

#define OSM_REPO_URI        "https://tile.openstreetmap.org/#Z/#X/#Y.png"
#define OSM_MIN_ZOOM        1
#define OSM_MAX_ZOOM        20
#define OSM_IMAGE_FORMAT    "png"

#define URI_MARKER_X    "#X"
#define URI_MARKER_Y    "#Y"
#define URI_MARKER_Z    "#Z"
#define URI_MARKER_S    "#S"
#define URI_MARKER_Q    "#Q"
#define URI_MARKER_Q0   "#W"
#define URI_MARKER_YS   "#U"
#define URI_MARKER_R    "#R"



MapSettings::MapSettings() {
}


MapSettings::~MapSettings() {
}


const MapSettings::Source& MapSettings::source(void) const {
	return source_;
}


void MapSettings::setSource(const MapSettings::Source &source) {
	source_ = source;
}


const int& MapSettings::zoom(void) const {
	return zoom_;
}


void MapSettings::setZoom(const int &zoom) {
	zoom_ = zoom;
}


void MapSettings::getBoundingBox(double *lat1, double *lon1, double *lat2, double *lon2) const {
	*lat1 = lat1_;
	*lon1 = lon1_;

	*lat2 = lat2_;
	*lon2 = lon2_;
}


void MapSettings::setBoundingBox(double lat1, double lon1, double lat2, double lon2) {
	lat1_ = lat1;
	lon1_ = lon1;

	lat2_ = lat2;
	lon2_ = lon2;
}


const std::string MapSettings::getFriendlyName(const MapSettings::Source &source) {
	switch (source) {
	case SourceNull:
		return "None";
	case SourceOpenStreetMap:
		return "OpenStreetMap I";
	case SourceOpenStreetMapRenderer:
		return "OpenStreetMap II";
	case SourceOpenAerialMap:
		return "OpenAerialMap";
	case SourceOpenCycleMap:
		return "OpenCycleMap";
	case SourceOpenTopoMap: 
		return "OpenTopoMap";
	case SourceOSMPublicTransport:
		return "Public Transport";
	case SourceOSMCTrails:
		return "OSMC Trails";
	case SourceMapsForFree:
		return "Maps-For-Free";
	case SourceGoogleStreet:
		return "Google Maps";
	case SourceGoogleSatellite:
		return "Google Satellite";
	case SourceGoogleHybrid:
		return "Google Hybrid";
	case SourceVirtualEarthStreet:
		return "Virtual Earth";
	case SourceVirtualEarthSatellite:
		return "Virtual Earth Satellite";
	case SourceVirtualEarthHybrid:
		return "Virtual Earth Hybrid";
	case SourceCount:
	default:
		return "";
	}

	return "";
}


const std::string MapSettings::getCopyright(const MapSettings::Source &source) {
	switch (source) {
	case SourceOpenStreetMap:
		// https://www.openstreetmap.org/copyright
		return "© OpenStreetMap contributors";
	case SourceOpenCycleMap:
		// http://www.thunderforest.com/terms/
		return "Maps © thunderforest.com, Data © osm.org/copyright";
	case SourceOSMPublicTransport:
		return "Maps © ÖPNVKarte, Data © OpenStreetMap contributors";
	case SourceMapsForFree:
		return "Maps © Maps-For-Free";
	case SourceOpenTopoMap:
		return "© OpenTopoMap (CC-BY-SA)";
	case SourceGoogleStreet:
		return "Map provided by Google";
	case SourceGoogleSatellite:
		return "Map provided by Google ";
	case SourceGoogleHybrid:
		return "Map provided by Google";
	case SourceVirtualEarthStreet:
		return "Map provided by Microsoft";
	case SourceVirtualEarthSatellite:
		return "Map provided by Microsoft";
	case SourceVirtualEarthHybrid:
		return "Map provided by Microsoft";
	default:
		return "";
	}

	return "";
}


// http://www.internettablettalk.com/forums/showthread.php?t=5209
// https://garage.maemo.org/plugins/scmsvn/viewcvs.php/trunk/src/maps.c?root=maemo-mapper&view=markup
// http://www.ponies.me.uk/maps/GoogleTileUtils.java
// http://www.mgmaps.com/cache/MapTileCacher.perl
const std::string MapSettings::getRepoURI(const MapSettings::Source &source) {
	switch (source) {
	case MapSettings::SourceNull:
		return "none://";
	case MapSettings::SourceOpenStreetMap:
		return OSM_REPO_URI;
	case MapSettings::SourceOpenAerialMap:
		// OpenAerialMap is down, offline till furthur notice
		// http://openaerialmap.org/pipermail/talk_openaerialmap.org/2008-December/000055.html
		return "";
	case MapSettings::SourceOpenStreetMapRenderer:
		// The Tile@Home serverhas been shut down.
		// return "http://tah.openstreetmap.org/Tiles/tile/#Z/#X/#Y.png";
		return "";
	case MapSettings::SourceOpenCycleMap:
		// return "http://c.andy.sandbox.cloudmade.com/tiles/cycle/#Z/#X/#Y.png";
		return "http://b.tile.opencyclemap.org/cycle/#Z/#X/#Y.png";
	case MapSettings::SourceOSMPublicTransport:
		return "http://tile.xn--pnvkarte-m4a.de/tilegen/#Z/#X/#Y.png";
	case MapSettings::SourceOSMCTrails:
		// Appears to be shut down
		return "";
	case MapSettings::SourceMapsForFree:
		return "https://maps-for-free.com/layer/relief/z#Z/row#Y/#Z_#X-#Y.jpg";
	case MapSettings::SourceOpenTopoMap:
		return "https://a.tile.opentopomap.org/#Z/#X/#Y.png";
	case MapSettings::SourceGoogleStreet:
		return "http://mt#R.google.com/vt/lyrs=m&hl=en&x=#X&s=&y=#Y&z=#Z";
	case MapSettings::SourceGoogleHybrid:
		return "http://mt#R.google.com/vt/lyrs=y&hl=en&x=#X&s=&y=#Y&z=#Z";
	case MapSettings::SourceGoogleSatellite:
		return "http://mt#R.google.com/vt/lyrs=s&hl=en&x=#X&s=&y=#Y&z=#Z";
	case MapSettings::SourceVirtualEarthStreet:
		return "http://a#R.ortho.tiles.virtualearth.net/tiles/r#W.jpeg?g=50";
	case MapSettings::SourceVirtualEarthSatellite:
		return "http://a#R.ortho.tiles.virtualearth.net/tiles/a#W.jpeg?g=50";
	case MapSettings::SourceVirtualEarthHybrid:
		return "http://a#R.ortho.tiles.virtualearth.net/tiles/h#W.jpeg?g=50";
	case MapSettings::SourceCount:
	default:
		return "";
	}

	return "";
}


int MapSettings::getMinZoom(const MapSettings::Source &source) {
	(void) source;

	return 1;
}


int MapSettings::getMaxZoom(const MapSettings::Source &source) {
	switch (source) {
	case MapSettings::SourceNull:
		return 18;
	case SourceOpenStreetMap:
		return 19;
	case SourceOpenCycleMap:
		return 18;
	case SourceOSMPublicTransport:
		return OSM_MAX_ZOOM;
	case SourceOpenStreetMapRenderer:
	case SourceOpenAerialMap:
	case SourceOpenTopoMap:
		return 17;
	case SourceGoogleStreet:
	case SourceGoogleSatellite:
	case SourceGoogleHybrid:
		return OSM_MAX_ZOOM;
	case SourceVirtualEarthStreet:
	case SourceVirtualEarthSatellite:
	case SourceVirtualEarthHybrid:
		return 19;
	case MapSettings::SourceOSMCTrails:
		return 15;
	case SourceMapsForFree:
		return 11;
	case MapSettings::SourceCount:
	default:
		return 17;
	}

	return 17;
}




Map::Map(const MapSettings &settings, struct event_base *evbase)
	: settings_(settings)
	, evbase_(evbase)
	, nbr_downloads_(0) {
	log_call();

	evcurl_ = EVCurl::init(evbase);
	
	evcurl_->setOption(CURLMOPT_MAXCONNECTS, 1);
	evcurl_->setOption(CURLMOPT_MAX_HOST_CONNECTIONS, 1);
	evcurl_->setOption(CURLMOPT_MAX_PIPELINE_LENGTH, 1);
}


Map::~Map() {
	log_call();

	delete evcurl_;
}


const MapSettings& Map::settings() const {
	log_call();

	return settings_;
}


Map * Map::create(const MapSettings &settings, struct event_base *evbase) {
	Map *map;

	log_call();

	map = new Map(settings, evbase);

	map->init();

	return map;
}


int Map::lat2pixel(int zoom, float lat) {
    float lat_m;
    int pixel_y;

    double latrad = lat * M_PI / 180.0;

	log_call();

    lat_m = atanh(sin(latrad));

    // the formula is some more notes
    // http://manialabs.wordpress.com/2013/01/26/converting-latitude-and-longitude-to-map-tile-in-mercator-projection/
    //
    // pixel_y = -(2^zoom * TILESIZE * lat_m) / 2PI + (2^zoom * TILESIZE) / 2
    pixel_y = -(int)( (lat_m * TILESIZE * (1 << zoom) ) / (2*M_PI)) +
        ((1 << zoom) * (TILESIZE/2) );

    return pixel_y;
}


int Map::lon2pixel(int zoom, float lon) {
    int pixel_x;

    double lonrad = lon * M_PI / 180.0;

	log_call();

    // the formula is
    //
    // pixel_x = (2^zoom * TILESIZE * lon) / 2PI + (2^zoom * TILESIZE) / 2
    pixel_x = (int)(( lonrad * TILESIZE * (1 << zoom) ) / (2*M_PI)) +
        ( (1 << zoom) * (TILESIZE/2) );

    return pixel_x;
}


std::string Map::buildURI(int zoom, int x, int y) {
	char s[16];

	int max_zoom = settings().getMaxZoom(settings().source());

	std::string uri = settings().getRepoURI(settings().source());

	log_call();

	if (std::strstr(uri.c_str(), URI_MARKER_X)) {
		snprintf(s, sizeof(s), "%d", x);
		uri = replace(uri, URI_MARKER_X, s);
	}

	if (std::strstr(uri.c_str(), URI_MARKER_Y)) {
		snprintf(s, sizeof(s), "%d", y);
		uri = replace(uri, URI_MARKER_Y, s);
	}

	if (std::strstr(uri.c_str(), URI_MARKER_Z)) {
		snprintf(s, sizeof(s), "%d", zoom);
		uri = replace(uri, URI_MARKER_Z, s);
	}

	if (std::strstr(uri.c_str(), URI_MARKER_S)) {
		snprintf(s, sizeof(s), "%d", max_zoom-zoom);
		uri = replace(uri, URI_MARKER_S, s);
	}

	if (std::strstr(uri.c_str(), URI_MARKER_Q)) {
//		map_convert_coords_to_quadtree_string(map, x, y, zoom, location, 't', "qrts");
//		uri = replace(uri, URI_MARKER_Q, location);
	}

	if (std::strstr(uri.c_str(), URI_MARKER_Q0)) {
//		map_convert_coords_to_quadtree_string(map, x, y, zoom, location, '\0', "0123");
//		uri = replace(uri, URI_MARKER_Q0, location);
	}

	if (std::strstr(uri.c_str(), URI_MARKER_YS)) {
	}

	if (std::strstr(uri.c_str(), URI_MARKER_R)) {
		snprintf(s, sizeof(s), "%d", (int) (random() % 4));
		uri = replace(uri, URI_MARKER_R, s);
	}

	if (std::strstr(uri.c_str(), "google.com")) {
	}

	return uri;
}


std::string Map::buildPath(int zoom, int x, int y) {
	std::ostringstream stream;

	(void) x;
	(void) y;

	stream << std::getenv("HOME");
	stream << "/.gpx2video/cache/" << zoom;

	return stream.str();
}


std::string Map::buildFilename(int zoom, int x, int y) {
	std::ostringstream stream;

	(void) zoom;

	stream << "tile_" << y << "_" << x << ".png";

	return stream.str();
}


void Map::init(void) {
	int zoom;

	double lat1, lon1;
	double lat2, lon2;

	std::string uri;

	Tile *tile;

	log_call();

	zoom = settings().zoom();
	settings().getBoundingBox(&lat1, &lon1, &lat2, &lon2);

	// Tiles:
	// +-------+-------+-------+ ..... +-------+
	// | x1,y1 |       |       |       | x2,y1 |
	// +-------+-------+-------+ ..... +-------+
	// |       |       |       |       |       |
	// +-------+-------+-------+ ..... +-------+
	// | x1,y2 |       |       |       | x2,y2 |
	// +-------+-------+-------+ ..... +-------+

	x1_ = floorf((float) Map::lon2pixel(zoom, lon1) / (float) TILESIZE);
	y1_ = floorf((float) Map::lat2pixel(zoom, lat1) / (float) TILESIZE);

	x2_ = floorf((float) Map::lon2pixel(zoom, lon2) / (float) TILESIZE) + 1;
	y2_ = floorf((float) Map::lat2pixel(zoom, lat2) / (float) TILESIZE) + 1;

	// Build each tile
	for (int y=y1_; y<y2_; y++) {
		for (int x=x1_; x<x2_; x++) {
			tile = new Tile(*this, zoom, x, y);
			tiles_.push_back(tile);
		}
	}
}


void Map::download(void) {
	std::string uri;

	log_call();

	log_notice("Download map from %s...", MapSettings::getFriendlyName(settings().source()).c_str());

	nbr_downloads_ = 1;

	// Build & download each tile
	for (Tile *tile : tiles_) {
//		log_info("Download tile: %s", tile->uri().c_str());

		tile->download();
	}
}


void Map::build(void) {
	int width, height;

	log_call();

	log_notice("Build map...");

	// Map size
	width = (x2_ - x1_) * TILESIZE;
	height = (y2_ - y1_) * TILESIZE;

	// Create map
	std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create("map.png");
	OIIO::ImageSpec outspec(width, height, 3);
	outspec.tile_width = TILESIZE;
	outspec.tile_height = TILESIZE;
	out->open("map.png", outspec);

	// Collapse echo tile
	for (Tile *tile : tiles_) {
		std::string filename = tile->path() + "/" + tile->filename();

		// Open tile image
		auto img = OIIO::ImageInput::open(filename.c_str());
		const OIIO::ImageSpec& spec = img->spec();
		OIIO::TypeDesc::BASETYPE type = (OIIO::TypeDesc::BASETYPE) spec.format.basetype;

		OIIO::ImageBuf buf(OIIO::ImageSpec(spec.width, spec.height, spec.nchannels, type));
		img->read_image(type, buf.localpixels());

		// Image over
		out->write_tile((tile->x() - x1_) * TILESIZE, (tile->y() - y1_) * TILESIZE, 0, type, buf.localpixels());
	}

	out->close();
}


void Map::draw(void) {
	log_call();
}


void Map::downloadProgress(Map::Tile &tile, double dltotal, double dlnow) {
	char buf[64];
	const char *label = "Download tile";

	Map& map = tile.map();

	int percent = (dltotal > 0) ? (int) (dlnow * 100 / dltotal) : 0;

	memset(buf, '.', 50);          
	memset(buf, '#', percent / 2); 
	buf[50] = '\0';

	if (percent == 100) 
		printf("\r  %s %d / %d [%s] DONE      ",          
				label, map.nbr_downloads_, (unsigned int) map.tiles_.size(), buf);
	else
		printf("\r  %s %d / %d [%s] %3d %%",
				label, map.nbr_downloads_, (unsigned int) map.tiles_.size(), buf, percent);
}


void Map::downloadComplete(Map::Tile &tile) {
	char buf[64];
	const char *label = "Download tile";

	Map& map = tile.map();

	memset(buf, '#', 50);          
	buf[50] = '\0';

	printf("\r  %s %d / %d [%s] DONE      ",          
			label, map.nbr_downloads_, (unsigned int) map.tiles_.size(), buf);

	map.nbr_downloads_++;

	if (map.nbr_downloads_ <= (unsigned int) map.tiles_.size())
		return;

	printf("\n");

	// Now build full map
	map.build();
}


Map::Tile::Tile(Map &map, int zoom, int x, int y)
	: map_(map)
	, zoom_(zoom)
	, x_(x)
	, y_(y) {
	fp_ = NULL;
	evtaskh_ = NULL;

	uri_ = map_.buildURI(zoom_, x_, y_);
	path_ = map_.buildPath(zoom_, x_, y_);
	filename_ = map_.buildFilename(zoom_, x_, y_);
}


Map::Tile::~Tile() {
}


Map& Map::Tile::map(void) {
	return map_;
}


const std::string& Map::Tile::uri(void) {
	return uri_;
}


const std::string& Map::Tile::path(void) {
	return path_;
}


const std::string& Map::Tile::filename(void) {
	return filename_;
}


int Map::Tile::downloadDebug(CURL *curl, curl_infotype type, char *ptr, size_t size, void *userdata) {
	(void) curl;
	(void) type;
	(void) userdata;

	fwrite(ptr, size, 1, stdout);

	return 0;
}


int Map::Tile::downloadProgress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	Map::Tile *tile = (Map::Tile *) clientp;

	(void) ultotal;
	(void) ulnow;

	Map::downloadProgress(*tile, dltotal, dlnow);

	return 0;
}


size_t Map::Tile::downloadWrite(char *ptr, size_t size, size_t nmemb, void *userdata) {
	Map::Tile *tile = (Map::Tile *) userdata;

	if (ptr == NULL)
		return 0;

	fwrite(ptr, size, nmemb, tile->fp_);

	return size * nmemb;
}


void Map::Tile::downloadComplete(EVCurlTask *evtaskh, CURLcode result, void *userdata) {
	Map::Tile *tile = (Map::Tile *) userdata;

	log_call();

	(void) evtaskh;
	(void) result;

	std::fclose(tile->fp_);

	tile->fp_ = NULL;
	tile->evtaskh_ = NULL;

	Map::downloadComplete(*tile);
}


bool Map::Tile::download(void) {
	std::string output = path_ + "/" + filename_;

	log_call();

	::mkdir(path_.c_str(), 0700);

	fp_ = std::fopen(output.c_str(), "w+");

	if (fp_ == NULL)
		return false;

	evtaskh_ = map_.evcurl()->download(uri_.c_str(), downloadComplete, this);

//	evtaskh_->setOption(CURLOPT_VERBOSE, 1L);
//	evtaskh_->setOption(CURLOPT_DEBUGFUNCTION, downloadDebug);
//	evtaskh_->setOption(CURLOPT_DEBUGDATA, this);

	evtaskh_->setOption(CURLOPT_NOPROGRESS, 0L);
	evtaskh_->setOption(CURLOPT_PROGRESSFUNCTION, downloadProgress);
	evtaskh_->setOption(CURLOPT_PROGRESSDATA, this);
	
	evtaskh_->setOption(CURLOPT_WRITEFUNCTION, downloadWrite);
	evtaskh_->setOption(CURLOPT_WRITEDATA, this);

	evtaskh_->setOption(CURLOPT_FOLLOWLOCATION, 1L);

	evtaskh_->setHeader("User-Agent: gpx2video");

	evtaskh_->perform();

	return true;
}
