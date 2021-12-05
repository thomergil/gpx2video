#include <iostream>
#include <cstdlib>
#include <string>

#include <string.h>
#include <getopt.h>

extern "C" {
#include <event2/event.h>
#include <libavcodec/avcodec.h>
}

#include "log.h"
#include "map.h"
#include "renderer.h"
#include "gpx2video.h"


namespace gpx2video {

static const struct option options[] = {
	{ "help",       no_argument,       0, 'h' },
	{ "verbose",    no_argument,       0, 'v' },
	{ "quiet",      no_argument,       0, 'q' },
	{ "duration",   required_argument, 0, 'd' },
	{ "media",      required_argument, 0, 'm' },
	{ "gpx",        required_argument, 0, 'g' },
	{ "output",     required_argument, 0, 'o' },
	{ "map-source", required_argument, 0, 's' },
	{ "map-zoom",   required_argument, 0, 'z' },
	{ "map-list",   no_argument,       0, 'l' },
	{ 0,            0,                 0, 0 }
};

static void print_usage(const std::string &name) {
	log_call();

	std::cout << "Usage: " << name << "%s [-v] -m=media -g=gpx -o=output command" << std::endl;
	std::cout << "       " << name << " -h" << std::endl;
	std::cout << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "\t- m, --media=file       : Input media file name" << std::endl;
	std::cout << "\t- g, --gpx=file         : GPX file name" << std::endl;
	std::cout << "\t- o, --output=file      : Output file name" << std::endl;
	std::cout << "\t- d, --duration         : Duration (in ms)" << std::endl;
	std::cout << "\t- f, --map-factor       : Map factor (default: 1.0)" << std::endl;
	std::cout << "\t- s, --map-source       : Map source" << std::endl;
	std::cout << "\t- z, --map-zoom         : Map zoom" << std::endl;
	std::cout << "\t- l, --map-list         : Dump supported map list" << std::endl;
	std::cout << "\t- v, --verbose          : Show trace" << std::endl;
	std::cout << "\t- q, --quiet            : Quiet mode" << std::endl;
	std::cout << "\t- h, --help             : Show this help screen" << std::endl;
	std::cout << std::endl;
	std::cout << "Command:" << std::endl;
	std::cout << "\t map   : Build map from gpx data" << std::endl;
	std::cout << "\t video : Process output video" << std::endl;

	return;
}

static void print_map_list(const std::string &name) {
	int i;

	log_call();

	std::cout << "Map list: " << name << std::endl;

	for (i=MapSettings::SourceNull; i != MapSettings::SourceCount; i++) {
		std::string name = MapSettings::getFriendlyName((MapSettings::Source) i);
		std::string copyright = MapSettings::getCopyright((MapSettings::Source) i);
		std::string uri = MapSettings::getRepoURI((MapSettings::Source) i);

		if (uri == "")
			continue;

		std::cout << "\t- " << i << ":\t" << name << " " << copyright << std::endl;
	}
}


//static void process(GPX2Video &app) {
//	int with_video = 1;
//	const std::string &mediafile = app.settings().mediafile();
//	const std::string &gpxfile = app.settings().gpxfile();
//	const std::string &outputfile = app.settings().outputfile();
//	int max_duration_ms = app.settings().maxDuration();
//	MapSettings::Source map_source = app.settings().mapsource();
//	int map_zoom = app.settings().mapzoom();
//
//	// Video utc start time
//	time_t start_time;
//
//	// GPX input file
//	GPXData data;
//
//	log_call();
//
//	GPX *gpx = GPX::open(gpxfile);
//
//	gpx->dump();
//
//
////	// Build map
////	GPXData::point p1, p2;
////	gpx->getBoundingBox(&p1, &p2);
////
////	MapSettings mapSettings;
////	mapSettings.setSource(map_source);
////	mapSettings.setZoom(map_zoom);
////	mapSettings.setBoundingBox(p1.lat, p1.lon, p2.lat, p2.lon);
////
////	Map *map = Map::create(mapSettings);
////	map->download();
//////	map->drawTrack(gpx);
//////	map->setPosition();
////
////	if (!with_video)
////		return;
//
//	// Probe input media
//	MediaContainer *container = Decoder::probe(mediafile);
////	MediaContainer *container = Decoder::probe("../video/clip.mp4");
////	MediaContainer *container = Decoder::probe("../video/GOPR1860.MP4");
//
//	// Set start time in GPX stream
//	start_time = container->startTime();
//	gpx->setStartTime(start_time);
//
//	// Retrieve audio & video streams
//	VideoStreamPtr video_stream = container->getVideoStream();
//	AudioStreamPtr audio_stream = container->getAudioStream();
//
//	// Audio & Video encoder settings
//	VideoParams video_params(video_stream->width(), video_stream->height(),
//		// av_make_q(1,  50), 
//		av_inv_q(video_stream->frameRate()),
//		video_stream->format(),
//		video_stream->nbChannels(),
//		video_stream->pixelAspectRatio(),
//		video_stream->interlacing());
//	AudioParams audio_params(audio_stream->sampleRate(),
//		audio_stream->channelLayout(),
//		audio_stream->format());
//
////	video_params.setTimeBase(video_stream->timeBase());
//	video_params.setPixelFormat(video_stream->pixelFormat());
//
//	EncoderSettings settings;
//	settings.setFilename(outputfile);
//	settings.setVideoParams(video_params, AV_CODEC_ID_H264);
//	settings.setVideoBitrate(4 * 1000 * 1000 * 8);
//	settings.setVideoMaxBitrate(2 * 1000 * 1000 * 16);
//	settings.setVideoBufferSize(4 * 1000 * 1000 / 2);
//	settings.setAudioParams(audio_params, AV_CODEC_ID_AAC);
//	settings.setAudioBitrate(44 * 1000);
//
//	// Open & decode input media
//	Decoder *decoder_video = Decoder::create();
//	decoder_video->open(video_stream);
//
//	Decoder *decoder_audio = Decoder::create();
//	decoder_audio->open(audio_stream);
//
//	// Open & encode output video
//	Encoder *encoder = Encoder::create(settings);
//	encoder->open();
//
//	// Renderer
//	Renderer *renderer = Renderer::create(app);
//
//	/**
//	 * Choose fps from command line (or from input stream
//	 * for each output frame, we can set easily the PTS value of output frame:
//	 *  pts = timestamp (in second) / av_q2d(timebase of output video stream)
//	 * We have to retrieve input frame in providing timestamp (in second).
//	 * At last, we can extract telemetry data at timestamp (in second).
//	 */
//	int64_t timecode;
//	int64_t timecode_ms;
//	int64_t frame_time = 0;
//
//	for (;;) {
//		FramePtr frame;
//
//		AVRational real_time;
//
//		real_time = av_mul_q(av_make_q(frame_time, 1), encoder->settings().videoParams().timeBase()); // timestamp_to_time(frame_time, time_base);
//
//		// Read audio data
//		frame = decoder_audio->retrieveAudio(audio_params, real_time);
//		
//		if (frame != NULL)
//			encoder->writeAudio(frame, real_time);
//
//		// Read video data
//		frame = decoder_video->retrieveVideo(real_time);
//
//		if (frame == NULL)
//			break;
//
//		timecode = frame->timestamp();
//		timecode_ms = timecode * av_q2d(video_stream->timeBase()) * 1000;
//
//		// Read GPX data
//		data = gpx->retrieveData(timecode_ms);
//
//		// Draw
//		renderer->draw(frame, data);
//
//		// Max 5 secondes
//		if (timecode_ms > max_duration_ms)
//			break;
//
//		// Dump frame info
//		char s[128];
//		const time_t t = start_time + (timecode_ms / 1000);
//		struct tm time;
//
//		localtime_r(&t, &time);
//
//		strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", &time);
//
//		printf("FRAME: %ld - PTS: %ld - TIMESTAMP: %ld ms - TIME: %s\n", 
//			frame_time, timecode, timecode_ms, s);
//
//		// Dump GPX data
//		data.dump();
//
//		real_time = av_mul_q(av_make_q(timecode, 1), video_stream->timeBase());
//
//		encoder->writeFrame(frame, real_time);
//
//		frame_time++;
//	}
//
//	encoder->close();
//	decoder_audio->close();
//	decoder_video->close();
//}

}; // namespace gpx2video



int GPX2Video::parseCommandLine(int argc, char *argv[]) {
	int index;
	int option;

	int with_video = 0;
	int verbose = 0;
	int map_zoom = 12;
	int max_duration_ms = 5 * 1000; // By default 5 seconds

	double map_factor = 1.0;

	MapSettings::Source map_source = MapSettings::SourceOpenStreetMap;

	char *gpxfile = NULL;
	char *mediafile = NULL;
	char *outputfile = NULL;

	const std::string name(argv[0]);

	log_call();

	for (;;) {
		index = 0;
		option = getopt_long(argc, argv, "hqvd:m:g:o:f:s:z:l", gpx2video::options, &index);

		if (option == -1) 
			break;

		switch (option) {
		case 0:
			std::cout << "option " << gpx2video::options[index].name;
			if (optarg)
				std::cout << " with arg " << optarg;
			std::cout << std::endl;
			break;
		case 'h':
			return -1;
			break;
		case 'l':
			return -2;
			break;
		case 'f':
			map_factor = strtod(optarg, NULL);
			break;
		case 'z':
			map_zoom = atoi(optarg);
			break;
		case 's':
			map_source = (MapSettings::Source) atoi(optarg);
			break;
		case 'q':
//			GPX2Video::setLogQuiet(true);
			break;
		case 'v':
			verbose++;
			break;
		case 'd':
			max_duration_ms = atoi(optarg);
			break;
		case 'm':
			if (mediafile != NULL) {
				std::cout << "'media' option is already set!" << std::endl;
				return -1;
			}
			mediafile = strdup(optarg);
			break;
		case 'g':
			if (gpxfile != NULL) {
				std::cout << "'gpx' option is already set!" << std::endl;
				return -1;
			}
			gpxfile = strdup(optarg);
			break;
		case 'o':
			if (outputfile != NULL) {
				std::cout << "'output' option is already set!" << std::endl;
				return -1;
			}
			outputfile = strdup(optarg);
			break;
		default:
			return -1;
			break;
		}
	}

	// getopt has consumed
	argc -= optind;
	argv += optind;
	optind = 0;

	// Check required options
	if (mediafile == NULL) {
		std::cout << name << ": option '--media' is required" << std::endl;
		return -1;
	}

	if (gpxfile == NULL) {
		std::cout << name << ": option '--gpx' is required" << std::endl;
		return -1;
	}

	if (outputfile == NULL) {
		std::cout << name << ": option '--output' is required" << std::endl;
		return -1;
	}

	if (argc == 1) {
		if (!strcmp(argv[0], "map")) {
			with_video = 0;
		}
		else if (!strcmp(argv[0], "video")) {
			with_video = 1;
		}
		else {
			std::cout << name << ": command '" << argv[0] << "' unknown" << std::endl;
			return -1;
		}
	}
	else
		with_video = 1;

	// Save app settings
	setSettings(GPX2Video::Settings(
		gpxfile,
		mediafile,
		outputfile,
		map_factor,
		map_zoom,
		max_duration_ms,
		map_source)
	);

	if (mediafile != NULL)
		free(mediafile);
	if (gpxfile != NULL)
		free(gpxfile);
	if (outputfile != NULL)
		free(outputfile);

	return with_video;
}


int main(int argc, char *argv[], char *envp[]) {
	int result;

	Map *map = NULL;
	Renderer *renderer = NULL;

	struct event_base *evbase;

	const std::string name(argv[0]);

	(void) envp;

	// Event loop
	evbase = event_base_new();

	// Baner info
	log_notice("gpx2video v%s", GPX2Video::version().c_str());

	// Init
	GPX2Video app(evbase);

	// Logs
	app.setLogLevel(AV_LOG_INFO);

	// Parse args
	result = app.parseCommandLine(argc, argv);
	if (result < -1)
		gpx2video::print_map_list(name);
	else if (result < 0)
		gpx2video::print_usage(name);
	if (result < 0)
		goto exit;

	// Create gpx2video map task
	map = app.buildMap();

	app.append(map);

	// Create gpx2video renderer task
	if (result == 1) {
		renderer = Renderer::create(app, map);

		app.append(renderer);
	}

	// Infinite loop
	app.exec();

//	// Process
//	gpx2video::process(app);

exit:
	event_base_free(evbase);

	exit(EXIT_SUCCESS);
}

