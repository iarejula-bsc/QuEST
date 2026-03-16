# @author Íñigo Aréjula Aísa
if (ENABLE_MALLEABILITY)
  target_sources(QuEST
    PRIVATE
    malleability.cpp
  )
endif()
