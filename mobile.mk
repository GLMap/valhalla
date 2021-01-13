SRC = \
	src/loki/matrix_action.cc \
	src/loki/height_action.cc \
	src/loki/transit_available_action.cc \
	src/loki/trace_route_action.cc \
	src/loki/node_search.cc \
	src/loki/search.cc \
	src/loki/worker.cc \
	src/loki/route_action.cc \
	src/loki/locate_action.cc \
	src/thor/timedep_forward.cc \
	src/thor/timedep_reverse.cc \
	src/thor/trafficalgorithm.cc \
	src/thor/trippathbuilder.cc \
	src/thor/matrix_action.cc \
	src/thor/route_matcher.cc \
	src/thor/bidirectional_astar.cc \
	src/thor/trace_attributes_action.cc \
	src/thor/trace_route_action.cc \
	src/thor/map_matcher.cc \
	src/thor/attributes_controller.cc \
	src/thor/isochrone_action.cc \
	src/thor/astar.cc \
	src/thor/multimodal.cc \
	src/thor/optimized_route_action.cc \
	src/thor/optimizer.cc \
	src/thor/isochrone.cc \
	src/thor/timedistancematrix.cc \
	src/thor/worker.cc \
	src/thor/route_action.cc \
	src/thor/costmatrix.cc \
	src/meili/topk_search.cc \
	src/meili/map_matcher.cc \
	src/meili/map_matcher_factory.cc \
	src/meili/routing.cc \
	src/meili/transition_cost_model.cc \
	src/meili/viterbi_search.cc \
	src/meili/candidate_search.cc \
	src/meili/match_route.cc \
	src/sif/pedestriancost.cc \
	src/sif/motorcyclecost.cc \
	src/sif/motorscootercost.cc \
	src/sif/autocost.cc \
	src/sif/bicyclecost.cc \
	src/sif/dynamiccost.cc \
	src/sif/transitcost.cc \
	src/sif/truckcost.cc \
	src/midgard/ellipse.cc \
	src/midgard/pointll.cc \
	src/midgard/gridded_data.cc \
	src/midgard/obb2.cc \
	src/midgard/aabb2.cc \
	src/midgard/linesegment2.cc \
	src/midgard/util.cc \
	src/midgard/tiles.cc \
	src/midgard/point2.cc \
	src/midgard/logging.cc \
	src/midgard/polyline2.cc \
	src/worker.cc \
	src/skadi/util.cc \
	src/skadi/sample.cc \
	src/tyr/height_serializer.cc \
	src/tyr/isochrone_serializer.cc \
	src/tyr/route_serializer.cc \
	src/tyr/matrix_serializer.cc \
	src/tyr/trace_serializer.cc \
	src/tyr/serializers.cc \
	src/tyr/transit_available_serializer.cc \
	src/tyr/locate_serializer.cc \
	src/odin/narrative_builder_factory.cc \
	src/odin/sign.cc \
	src/odin/maneuversbuilder.cc \
	src/odin/directionsbuilder.cc \
	src/odin/util.cc \
	src/odin/maneuver.cc \
	src/odin/narrative_dictionary.cc \
	src/odin/transitrouteinfo.cc \
	src/odin/worker.cc \
	src/odin/narrativebuilder.cc \
	src/odin/signs.cc \
	src/odin/enhancedtrippath.cc \
	src/baldr/compression_utils.cc \
	src/baldr/datetime.cc \
	src/baldr/verbal_text_formatter_us.cc \
	src/baldr/verbal_text_formatter_us_tx.cc \
	src/baldr/verbal_text_formatter_us_co.cc \
	src/baldr/pathlocation.cc \
	src/baldr/streetnames_us.cc \
	src/baldr/graphtile.cc \
	src/baldr/graphtileheader.cc \
	src/baldr/verbal_text_formatter.cc \
	src/baldr/verbal_text_formatter_factory.cc \
	src/baldr/admin.cc \
	src/baldr/curler.cc \
	src/baldr/directededge.cc \
	src/baldr/location.cc \
	src/baldr/transitdeparture.cc \
	src/baldr/laneconnectivity.cc \
	src/baldr/transitroute.cc \
	src/baldr/streetname_us.cc \
	src/baldr/streetnames.cc \
	src/baldr/streetname.cc \
	src/baldr/accessrestriction.cc \
	src/baldr/tilehierarchy.cc \
	src/baldr/nodeinfo.cc \
	src/baldr/streetnames_factory.cc \
	src/baldr/transittransfer.cc \
	src/baldr/graphid.cc \
	src/baldr/edgetracker.cc \
	src/baldr/merge.cc \
	src/baldr/turn.cc \
	src/baldr/edgeinfo.cc \
	src/baldr/transitschedule.cc \
	src/baldr/edge_elevation.cc \
	src/baldr/graphreader.cc \
	src/baldr/connectivity_map.cc

GENERATED_HEADERS = \
	genfiles/valhalla/valhalla.h \
	genfiles/config.h \
	genfiles/date_time_zonespec.h \
	genfiles/graph_lua_proc.h \
	genfiles/admin_lua_proc.h \
	genfiles/locales.h \
	genfiles/valhalla/proto/tripcommon.pb.h \
	genfiles/valhalla/proto/trippath.pb.h \
	genfiles/valhalla/proto/directions_options.pb.h \
	genfiles/valhalla/proto/tripdirections.pb.h \
	genfiles/valhalla/proto/directions.pb.h

GENERATED_SOURCES = \
	genfiles/proto/tripcommon.pb.cc \
	genfiles/proto/trippath.pb.cc \
	genfiles/proto/directions_options.pb.cc \
	genfiles/proto/tripdirections.pb.cc \
	genfiles/proto/directions.pb.cc

THRID_PARTY_SOURCES = \
	third_party/date/src/tz.cpp

SRC := $(GENERATED_SOURCES) $(SRC)

OBJ = $(SRC:.cc=.o) $(THRID_PARTY_SOURCES:.cpp=.o)
LIB = libvalhalla.a
FLAGS = -std=c++17 -DMOBILE -DNDEBUG=1 -DUSE_STD_REGEX=1 -DRAPIDJSON_HAS_STDSTRING=1 -DPACKAGE_VERSION="\"2.7.0\"" \
 -I. -Ivalhalla -Igenfiles -Ithird_party/rapidjson/include -Ithird_party/date/include
PROTOC = ../build/macOS/x86_64/bin/protoc

.cc.o:
	$(CXX) $(FLAGS) $(CPPFLAGS) ${CXXFLAGS} -c $< -o $@

.cpp.o:
	$(CXX) $(FLAGS) $(CPPFLAGS) ${CXXFLAGS} -c $< -o $@

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) cr $(LIB) $(OBJ)

$(OBJ): $(GENERATED_HEADERS)

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

# protobuf sources
genfiles/proto:
	mkdir -p genfiles/proto
genfiles/proto/tripcommon.pb.cc: genfiles/proto
	$(PROTOC) -Iproto --cpp_out=genfiles/proto proto/tripcommon.proto
genfiles/proto/trippath.pb.cc: genfiles/proto
	$(PROTOC) -Iproto --cpp_out=genfiles/proto proto/trippath.proto
genfiles/proto/directions_options.pb.cc: genfiles/proto
	$(PROTOC) -Iproto --cpp_out=genfiles/proto proto/directions_options.proto
genfiles/proto/tripdirections.pb.cc: genfiles/proto
	$(PROTOC) -Iproto --cpp_out=genfiles/proto proto/tripdirections.proto
genfiles/proto/directions.pb.cc: genfiles/proto
	$(PROTOC) -Iproto --cpp_out=genfiles/proto proto/directions.proto

genfiles/valhalla/proto:
	mkdir -p genfiles/valhalla/proto

genfiles/valhalla/proto/tripcommon.pb.h: genfiles/valhalla/proto genfiles/proto/tripcommon.pb.cc
	cp genfiles/proto/tripcommon.pb.h genfiles/valhalla/proto
genfiles/valhalla/proto/trippath.pb.h: genfiles/valhalla/proto genfiles/proto/trippath.pb.cc
	cp genfiles/proto/trippath.pb.h genfiles/valhalla/proto
genfiles/valhalla/proto/directions_options.pb.h: genfiles/valhalla/proto genfiles/proto/directions_options.pb.cc
	cp genfiles/proto/directions_options.pb.h genfiles/valhalla/proto
genfiles/valhalla/proto/tripdirections.pb.h: genfiles/valhalla/proto genfiles/proto/tripdirections.pb.cc
	cp genfiles/proto/tripdirections.pb.h genfiles/valhalla/proto
genfiles/valhalla/proto/directions.pb.h: genfiles/valhalla/proto genfiles/proto/directions.pb.cc
	cp genfiles/proto/directions.pb.h genfiles/valhalla/proto

.PHONY: install clean

ifndef PREFIX
PREFIX = /usr/local
endif

install: $(LIB)
	mkdir -p ${PREFIX}/lib ${PREFIX}/include/valhalla
	cp $(LIB) ${PREFIX}/lib
	cp genfiles/valhalla/valhalla.h ${PREFIX}/include/valhalla
	cp genfiles/config.h ${PREFIX}/include/valhalla
	cp valhalla/filesystem.h ${PREFIX}/include/valhalla
	cp valhalla/worker.h ${PREFIX}/include/valhalla
	cp -r valhalla/loki ${PREFIX}/include/valhalla
	cp -r genfiles/valhalla/proto ${PREFIX}/include/valhalla
	cp -r valhalla/thor ${PREFIX}/include/valhalla	
	cp -r valhalla/tyr ${PREFIX}/include/valhalla	
	cp -r valhalla/meili ${PREFIX}/include/valhalla
	cp -r valhalla/sif ${PREFIX}/include/valhalla
	cp -r valhalla/midgard ${PREFIX}/include/valhalla
	cp -r valhalla/skadi ${PREFIX}/include/valhalla
	cp -r valhalla/odin ${PREFIX}/include/valhalla
	cp -r valhalla/baldr ${PREFIX}/include/valhalla
	cp -r third_party/date/include/date ${PREFIX}/include

clean:
	rm -f $(OBJ) $(GENERATED_SOURCES) $(GENERATED_HEADERS) $(LIB)
