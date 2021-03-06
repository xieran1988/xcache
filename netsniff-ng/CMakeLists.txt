PROJECT(netsniff-ng C)

SET(BUILD_STRING "generic")
FIND_PACKAGE(Threads)
FIND_PACKAGE(Libnl)
FIND_PACKAGE(PythonLibs)
INCLUDE(Pod2Man)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
INCLUDE_DIRECTORIES(/usr/include/glib-2.0 /usr/lib/i386-linux-gnu/glib-2.0/include/)

IF (CMAKE_HAVE_PTHREAD_CREATE AND LIBNL_FOUND)
	ADD_EXECUTABLE(${PROJECT_NAME} 	../hash.c
					../dissector.c
					../dissector_eth.c
					../dissector_80211.c
					../proto_arp.c
					../proto_ethernet.c
					../proto_framectl.c
					../proto_icmpv4.c
					../proto_icmpv6.c
					../proto_igmp.c
					../proto_ip_authentication_hdr.c
					../proto_ip_esp.c
					../proto_ipv4.c
					../proto_ipv6.c
					../proto_ipv6_dest_opts.c
					../proto_ipv6_fragm.c
					../proto_ipv6_hop_by_hop.c
					../proto_ipv6_in_ipv4.c
					../proto_ipv6_mobility_hdr.c
					../proto_ipv6_no_nxt_hdr.c
					../proto_ipv6_routing.c
					../proto_none.c
					../proto_tcp.c
					../proto_udp.c
					../proto_vlan.c
					../proto_vlan_q_in_q.c
					../proto_mpls_unicast.c
					../xmalloc.c
					../oui.c
					../xio.c
					../xsys.c
					../xstring.c
					../bpf.c
					../mac80211.c
					../pcap.c
					../pcap_rw.c
					../pcap_sg.c
					../pcap_mmap.c
					../ring_rx.c
					../ring_tx.c
					../mtrand.c
					../tprintf.c
					../xcache_cap.c
					../xcache_queue.c
					../netsniff-ng.c)
	ADD_DEFINITIONS(-DPROGNAME_STRING="${PROJECT_NAME}"
		-DVERSION_STRING="${VERSION}"
		-DBUILD_STRING="${BUILD_STRING}"
		-D_FILE_OFFSET_BITS=64
		-D_LARGEFILE_SOURCE
		-D_LARGEFILE64_SOURCE
		-D__WITH_PROTOS)
	POD2MAN(${CMAKE_SOURCE_DIR}/netsniff-ng.c netsniff-ng 8)
	TARGET_LINK_LIBRARIES(netsniff-ng ${CMAKE_THREAD_LIBS_INIT} ${LIBNL_LIBRARIES} libglib-2.0.so ${PYTHON_LIBRARIES})
	INSTALL(FILES ${rules_files} DESTINATION ${CONFIG_INSTALL_PATH}/rules/)
	INSTALL(FILES ../conf/ether.conf ../conf/tcp.conf ../conf/udp.conf ../conf/oui.conf DESTINATION ${CONFIG_INSTALL_PATH})
	INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${EXECUTABLE_INSTALL_PATH})
ELSE(CMAKE_HAVE_PTHREAD_CREATE AND LIBNL_FOUND)
	MESSAGE("pthread or libnl is missing on target. Skipping ${PROJECT_NAME} build.")
ENDIF(CMAKE_HAVE_PTHREAD_CREATE AND LIBNL_FOUND)
