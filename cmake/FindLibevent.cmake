include (FindPackageHandleStandardArgs)

find_path     (LIBEVENT_INCLUDE_DIR NAMES event2/event.h)
find_library  (LIBEVENT_LIBRARY     NAMES event)
find_library  (LIBEVENT_CORE        NAMES event_core)
find_library  (LIBEVENT_EXTRA       NAMES event_extra)
find_library (LIBEVENT_THREAD   NAMES event_pthreads)
#find_library (LIBEVENT_SSL      NAMES event_openssl)

set (LIBEVENT_INCLUDE_DIRS ${LIBEVENT_INCLUDE_DIR})
set (LIBEVENT_LIBRARIES
        ${LIBEVENT_LIBRARY}
        ${LIBEVENT_SSL}
        ${LIBEVENT_CORE}
        ${LIBEVENT_EXTRA}
        ${LIBEVENT_THREAD}
        ${LIBEVENT_EXTRA})
find_package_handle_standard_args (LIBEVENT DEFAULT_MSG LIBEVENT_LIBRARIES LIBEVENT_INCLUDE_DIR)
mark_as_advanced(LIBEVENT_INCLUDE_DIRS LIBEVENT_LIBRARIES)
