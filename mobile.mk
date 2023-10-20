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
	src/loki/locate_action.cc \
	src/loki/matrix_action.cc \
	src/loki/node_search.cc \
	src/loki/polygon_search.cc \
	src/loki/reach.cc \
	src/loki/route_action.cc \
	src/loki/search.cc \
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
	src/thor/isochrone_action.cc \
	src/thor/isochrone.cc \
	src/thor/map_matcher.cc \
	src/thor/matrix_action.cc \
	src/thor/multimodal.cc \
	src/thor/optimized_route_action.cc \
	src/thor/optimizer.cc \
	src/thor/route_action.cc \
	src/thor/route_matcher.cc \
	src/thor/timedistancebssmatrix.cc \
	src/thor/timedistancematrix.cc \
	src/thor/trace_attributes_action.cc \
	src/thor/trace_route_action.cc \
	src/thor/triplegbuilder.cc \
	src/thor/unidirectional_astar.cc \
	src/thor/worker.cc \
	src/tyr/height_serializer.cc \
	src/tyr/isochrone_serializer.cc \
	src/tyr/locate_serializer.cc \
	src/tyr/matrix_serializer.cc \
	src/tyr/route_serializer.cc \
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
	genfiles/valhalla/proto/incidents.pb.cc \
	genfiles/valhalla/proto/info.pb.cc \
	genfiles/valhalla/proto/options.pb.cc \
	genfiles/valhalla/proto/sign.pb.cc \
	genfiles/valhalla/proto/status.pb.cc \
	genfiles/valhalla/proto/transit.pb.cc \
	genfiles/valhalla/proto/transit_fetch.pb.cc \
	genfiles/valhalla/proto/trip.pb.cc

THRID_PARTY_CPP_SOURCES = \
	third_party/date/src/tz.cpp

THRID_PARTY_C_SOURCES = \
	third_party/lz4/lib/lz4.c \
    third_party/lz4/lib/lz4hc.c \
    third_party/lz4/lib/lz4frame.c \
    third_party/lz4/lib/xxhash.c

ifeq "$(LIBS_PLATFORM)" "catalyst"
	IOS_SOURCES := third_party/date/src/ios.mm
	LDFLAGS := $(LDFLAGS) -framework CoreFoundation
else ifeq "$(LIBS_PLATFORM)" "android"
	LDFLAGS := $(LDFLAGS) -landroid -llog
endif

SRC := $(GENERATED_SOURCES) $(SRC)
OBJ = $(SRC:.cc=.o) $(THRID_PARTY_CPP_SOURCES:.cpp=.o) $(THRID_PARTY_C_SOURCES:.c=.o) $(IOS_SOURCES:.mm=.o)
LIB = libvalhalla.a
MICRO_SRC = micro.cpp
MICRO_OBJ = $(MICRO_SRC:.cpp=.o)
MICRO_LIB = libvalhalla_micro.a

CXXFLAGS += -std=c++17 -DMOBILE -DNDEBUG=1 -DUSE_STD_REGEX=1 -DRAPIDJSON_HAS_STDSTRING=1 \
 -I. -Ivalhalla -Igenfiles -Igenfiles/valhalla \
 -Ithird_party/cpp-statsd-client/include \
 -Ithird_party/robin-hood-hashing/src/include \
 -Ithird_party/rapidjson/include \
 -Ithird_party/lz4/lib \
 -Ithird_party/date/include
PROTOC = ../build/macOS/x86_64/bin/protoc

.SUFFIXES: .cc .cpp .mm .proto

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

.cc.o:
	$(CXX) $(FLAGS) $(CPPFLAGS) ${CXXFLAGS} -c $< -o $@

.cpp.o:
	$(CXX) $(FLAGS) $(CPPFLAGS) ${CXXFLAGS} -c $< -o $@

.mm.o:
	$(CXX) $(FLAGS) $(CPPFLAGS) ${CXXFLAGS} -x objective-c++ -c $< -o $@

all: $(MICRO_LIB)

$(OBJ): $(GENERATED_HEADERS)

$(MICRO_LIB): $(OBJ) $(MICRO_OBJ)
	$(AR) cr $(MICRO_LIB) $(MICRO_OBJ) $(OBJ)

genfiles:
	mkdir -p genfiles

genfiles/date_time_zonespec.h: genfiles date_time/*
	./preBuild.sh
genfiles/admin_lua_proc.h: genfiles lua/admin.lua
	cmake -P cmake/Binary2Header.cmake lua/admin.lua genfiles/admin_lua_proc.h --variable-name lua_admin_lua
genfiles/graph_lua_proc.h: genfiles lua/graph.lua
	cmake -P cmake/Binary2Header.cmake lua/graph.lua genfiles/graph_lua_proc.h --variable-name lua_graph_lua
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

install: $(MICRO_LIB)
	mkdir -p ${PREFIX}/lib ${PREFIX}/include/valhalla
	cp $(MICRO_LIB) ${PREFIX}/lib
	cp micro.h ${PREFIX}/include/valhalla

clean:
	rm -f $(OBJ) $(GENERATED_SOURCES) $(GENERATED_HEADERS) $(LIB) $(MICRO_LIB) $(MICRO_OBJ)
