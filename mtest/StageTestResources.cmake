# GUI tests need the same factory resources and permissions as an installation.
# In particular, writable factory workspaces bypass the "edited" copy behavior.
if (NOT DEFINED SOURCE_DIR OR NOT DEFINED BUILD_DIR OR NOT DEFINED RESOURCE_DIR)
      message(FATAL_ERROR "SOURCE_DIR, BUILD_DIR and RESOURCE_DIR are required")
endif()
file(COPY
      "${BUILD_DIR}/share/workspaces/Basic.workspace"
      "${BUILD_DIR}/share/workspaces/Advanced.workspace"
      DESTINATION "${RESOURCE_DIR}/workspaces"
      FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
file(COPY "${SOURCE_DIR}/share/styles"
      DESTINATION "${RESOURCE_DIR}"
      FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
