SRC = \
	src/baldr/accessrestriction.cc \
	src/baldr/admin.cc \
	src/baldr/attributes_controller.cc \
	src/baldr/compression_utils.cc \
	src/baldr/connectivity_map.cc \
	src/baldr/curler.cc \
	src/baldr/datetime.cc \
	src/baldr/directededge.cc \
	src/baldr/edgeinfo.cc \
	src/baldr/edgetracker.cc \
	src/baldr/graphid.cc \
	src/baldr/graphreader.cc \
	src/baldr/graphtile.cc \
	src/baldr/graphtileheader.cc \
	src/baldr/laneconnectivity.cc \
	src/baldr/location.cc \
	src/baldr/merge.cc \
	src/baldr/nodeinfo.cc \
	src/baldr/pathlocation.cc \
	src/baldr/predictedspeeds.cc \
	src/baldr/streetname_us.cc \
	src/baldr/streetname.cc \
	src/baldr/streetnames_factory.cc \
	src/baldr/streetnames_us.cc \
	src/baldr/streetnames.cc \
	src/baldr/tilehierarchy.cc \
	src/baldr/timedomain.cc \
	src/baldr/transitdeparture.cc \
	src/baldr/transitroute.cc \
	src/baldr/transitschedule.cc \
	src/baldr/transittransfer.cc \
	src/baldr/turn.cc \
	src/baldr/verbal_text_formatter_factory.cc \
	src/baldr/verbal_text_formatter_us_co.cc \
	src/baldr/verbal_text_formatter_us_tx.cc \
	src/baldr/verbal_text_formatter_us.cc \
	src/baldr/verbal_text_formatter.cc \
	src/loki/height_action.cc \
	src/loki/isochrone_action.cc \
	src/loki/locate_action.cc \
	src/loki/matrix_action.cc \
	src/loki/node_search.cc \
	src/loki/polygon_search.cc \
	src/loki/reach.cc \
	src/loki/route_action.cc \
	src/loki/search.cc \
	src/loki/status_action.cc \
	src/loki/trace_route_action.cc \
	src/loki/transit_available_action.cc \
	src/loki/worker.cc \
	src/meili/candidate_search.cc \
	src/meili/config.cc \
	src/meili/geometry_helpers.cc \
	src/meili/map_matcher_factory.cc \
	src/meili/map_matcher.cc \
	src/meili/match_route.cc \
	src/meili/routing.cc \
	src/meili/topk_search.cc \
	src/meili/transition_cost_model.cc \
	src/meili/viterbi_search.cc \
	src/midgard/aabb2.cc \
	src/midgard/ellipse.cc \
	src/midgard/linesegment2.cc \
	src/midgard/logging.cc \
	src/midgard/obb2.cc \
	src/midgard/point_tile_index.cc \
	src/midgard/point2.cc \
	src/midgard/pointll.cc \
	src/midgard/polyline2.cc \
	src/midgard/tiles.cc \
	src/midgard/util.cc \
	src/mjolnir/timeparsing.cc \
	src/odin/directionsbuilder.cc \
	src/odin/enhancedtrippath.cc \
	src/odin/maneuver.cc \
	src/odin/maneuversbuilder.cc \
	src/odin/markup_formatter.cc \
	src/odin/narrative_builder_factory.cc \
	src/odin/narrative_dictionary.cc \
	src/odin/narrativebuilder.cc \
	src/odin/sign.cc \
	src/odin/signs.cc \
	src/odin/transitrouteinfo.cc \
	src/odin/util.cc \
	src/odin/worker.cc \
	src/proto_conversions.cc \
	src/sif/autocost.cc \
	src/sif/bicyclecost.cc \
	src/sif/dynamiccost.cc \
	src/sif/hierarchylimits.cc \
	src/sif/motorcyclecost.cc \
	src/sif/motorscootercost.cc \
	src/sif/nocost.cc \
	src/sif/pedestriancost.cc \
	src/sif/recost.cc \
	src/sif/transitcost.cc \
	src/sif/truckcost.cc \
	src/skadi/sample.cc \
	src/skadi/util.cc \
	src/thor/alternates.cc \
	src/thor/astar_bss.cc \
	src/thor/bidirectional_astar.cc \
	src/thor/centroid.cc \
	src/thor/costmatrix.cc \
	src/thor/dijkstras.cc \
	src/thor/expansion_action.cc \
	src/thor/isochrone_action.cc \
	src/thor/isochrone.cc \
	src/thor/map_matcher.cc \
	src/thor/matrix_action.cc \
	src/thor/multimodal.cc \
	src/thor/optimized_route_action.cc \
	src/thor/optimizer.cc \
	src/thor/route_action.cc \
	src/thor/route_matcher.cc \
	src/thor/status_action.cc \
	src/thor/timedistancebssmatrix.cc \
	src/thor/timedistancematrix.cc \
	src/thor/trace_attributes_action.cc \
	src/thor/trace_route_action.cc \
	src/thor/triplegbuilder.cc \
	src/thor/unidirectional_astar.cc \
	src/thor/worker.cc \
	src/tyr/actor.cc \
	src/tyr/expansion_serializer.cc \
	src/tyr/height_serializer.cc \
	src/tyr/isochrone_serializer.cc \
	src/tyr/locate_serializer.cc \
	src/tyr/matrix_serializer.cc \
	src/tyr/route_serializer_osrm.cc \
	src/tyr/route_serializer_valhalla.cc \
	src/tyr/route_serializer.cc \
	src/tyr/route_summary_cache.cc \
	src/tyr/serializers.cc \
	src/tyr/trace_serializer.cc \
	src/tyr/transit_available_serializer.cc \
	src/worker.cc

GENERATED_HEADERS = \
	genfiles/admin_lua_proc.h \
	genfiles/config.h \
	genfiles/date_time_zonespec.h \
	genfiles/graph_lua_proc.h \
	genfiles/locales.h \
	genfiles/valhalla/valhalla.h

GENERATED_SOURCES = \
	genfiles/valhalla/proto/matrix.pb.cc \
	genfiles/valhalla/proto/api.pb.cc \
	genfiles/valhalla/proto/common.pb.cc \
	genfiles/valhalla/proto/directions.pb.cc \
	genfiles/valhalla/proto/expansion.pb.cc \
	genfiles/valhalla/proto/incidents.pb.cc \
	genfiles/valhalla/proto/info.pb.cc \
	genfiles/valhalla/proto/isochrone.pb.cc \
	genfiles/valhalla/proto/options.pb.cc \
	genfiles/valhalla/proto/sign.pb.cc \
	genfiles/valhalla/proto/status.pb.cc \
	genfiles/valhalla/proto/transit.pb.cc \
	genfiles/valhalla/proto/transit_fetch.pb.cc \
	genfiles/valhalla/proto/trip.pb.cc

THRID_PARTY_CPP_SOURCES = \
	third_party/date/src/tz.cpp

ifeq "$(LIBS_PLATFORM)" "android"
	LDFLAGS := $(LDFLAGS) -llz4 -lprotobuf-lite -lz -landroid -llog
else
	IOS_SOURCES := third_party/date/src/ios.mm
	LDFLAGS := $(LDFLAGS) -llz4 -lprotobuf-lite -lz -framework CoreFoundation
endif

SRC := $(GENERATED_SOURCES) $(SRC)
MICRO_SRC = micro.cpp
OBJ = $(SRC:.cc=.o) $(THRID_PARTY_CPP_SOURCES:.cpp=.o) $(IOS_SOURCES:.mm=.o) $(MICRO_SRC:.cpp=.o)

LIB = libvalhalla.a
MICRO_LIB = libvalhalla_micro.a
MICRO_DYNAMIC = libvalhalla_micro.dylib

CXXFLAGS += -std=c++17 -DMOBILE -DNDEBUG=1 -DUSE_STD_REGEX=1 -DRAPIDJSON_HAS_STDSTRING=1 \
 -I. -Ivalhalla -Igenfiles -Igenfiles/valhalla \
 -Ithird_party/cpp-statsd-client/include \
 -Ithird_party/robin-hood-hashing/src/include \
 -Ithird_party/rapidjson/include \
 -Ithird_party/date/include
PROTOC = ../build/macOS/arm64/bin/protoc

.SUFFIXES: .cc .cpp .mm .proto

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

.cc.o:
	$(CXX) $(FLAGS) $(CPPFLAGS) ${CXXFLAGS} -c $< -o $@

.cpp.o:
	$(CXX) $(FLAGS) $(CPPFLAGS) ${CXXFLAGS} -c $< -o $@

.mm.o:
	$(CXX) $(FLAGS) $(CPPFLAGS) ${CXXFLAGS} -x objective-c++ -c $< -o $@

all: $(MICRO_LIB) $(MICRO_DYNAMIC)

$(OBJ): $(GENERATED_HEADERS)

prepare_objs: $(OBJ)
	@mkdir -p objs
	@for obj in $(OBJ); do \
		new_name=$$(echo $$obj | sed 's/\//_/g'); \
		mv $$obj objs/$$new_name; \
	done

$(MICRO_LIB): prepare_objs
	$(AR) cr $(MICRO_LIB) objs/*.o

# build dynamic version to make sure, we have all symbols
$(MICRO_DYNAMIC): prepare_objs
	$(CXX) -shared -o $(MICRO_DYNAMIC) objs/*.o $(LDFLAGS)

genfiles:
	mkdir -p genfiles

genfiles/date_time_zonespec.h: genfiles date_time/*
	./preBuild.sh
genfiles/admin_lua_proc.h: genfiles lua/admin.lua
	cmake -P cmake/ValhallaBin2Header.cmake lua/admin.lua genfiles/admin_lua_proc.h --variable-name lua_admin_lua
genfiles/graph_lua_proc.h: genfiles lua/graph.lua
	cmake -P cmake/ValhallaBin2Header.cmake lua/graph.lua genfiles/graph_lua_proc.h --variable-name lua_graph_lua
genfiles/locales.h: genfiles locales/*.json
	-cd locales && ./make_locales.sh *.json > ../genfiles/locales.h
genfiles/config.h: genfiles
	touch genfiles/config.h

genfiles/valhalla:
	mkdir -p genfiles/valhalla
genfiles/valhalla/valhalla.h: genfiles/valhalla
	touch genfiles/valhalla/valhalla.h

genfiles/valhalla/proto:
	mkdir -p genfiles/valhalla/proto

# protobuf sources
genfiles/valhalla/proto/%.pb.cc: proto/%.proto genfiles/valhalla/proto
	$(PROTOC) -Iproto --cpp_out=genfiles/valhalla/proto $<

.PRECIOUS: genfiles/valhalla/proto/%.pb.cc genfiles/locales.h
.PHONY: install clean

ifndef PREFIX
PREFIX = /usr/local
endif

install: $(MICRO_LIB) $(MICRO_DYNAMIC)
	mkdir -p $(PREFIX)/lib $(PREFIX)/include/valhalla
	cp $(MICRO_LIB) $(PREFIX)/lib
	cp micro.h $(PREFIX)/include/valhalla
	rm -rf ../../glmap/Resources/framework/tzdata/*
	files=$$(find third_party/tz -type f -regex "[0-9a-z_/]*\.tab" -o -regex ".*/tz/[0-9a-z_/]*"); \
	for file in $$files; do \
		cp $$file ../../glmap/Resources/framework/tzdata/; \
	done
	#copy version from the NEWS file
	head -n 3 third_party/tz/NEWS > ../../glmap/Resources/framework/tzdata/NEWS

clean:
	@rm -f $(OBJ) $(GENERATED_SOURCES) $(GENERATED_HEADERS) $(LIB) $(MICRO_LIB) $(MICRO_DYNAMIC) $(MICRO_OBJ)
	@rm -rf objs
