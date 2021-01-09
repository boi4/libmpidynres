#include "comm.h"

#include <mpi.h>

#include "mpidynres.h"
#include "util.h"

/**
 * @brief      Free MPI datatypes used for communication
 *
 * @details    This function will free all mpi datatypes that were created when
 * calling get_<datatype_name>_datatype() except for the pset
 */
void free_all_mpi_datatypes() {
  // pset type is freed in the applicaion
  MPI_Datatype types[] = {
      get_init_info_datatype(),
      get_idle_command_datatype(),
      get_uri_op_datatype(),
      get_rc_datatype(),
  };
  for (size_t i = 0; i < COUNT_OF(types); i++) {
    MPI_Type_free(&types[i]);
  }
}

/**
 * @brief      Get mpi datatype that can send an MPIDYNRES_idle_command struct
 *
 * @details    free_all_mpi_datatypes has to be called when this function was
 * used
 *
 * @return     mpi datatype that can send an MPIDYNRES_idle_command struct with
 */
MPI_Datatype get_idle_command_datatype() {
  static MPI_Datatype result = NULL;

  static int const lengths[] = {
      1,  // one enum (int)
  };
  static MPI_Aint const displacements[] = {
      offsetof(MPIDYNRES_idle_command, command_type),
  };
  static MPI_Datatype const types[] = {
      MPI_INT,
  };

  if (result == NULL) {
    assert(COUNT_OF(lengths) == COUNT_OF(displacements) &&
           COUNT_OF(displacements) == COUNT_OF(types));

    MPI_Type_create_struct(COUNT_OF(lengths), lengths, displacements, types,
                           &result);
    MPI_Type_commit(&result);
  }
  return result;
}

/**
 * @brief      Get mpi datatype that can send an MPIDYNRES_pset struct with capacity cap
 *
 * @details    The datatype returned by this function hass to be free using
 * MPI_Type_free
 *
 * @param cap  The capacity of the pset
 *
 * @return     mpi datatype that can send an MPIDYNRES_uri_pset struct with capacity cap
 */
MPI_Datatype get_pset_datatype(size_t cap) {
  static MPI_Datatype result = NULL;

  static MPI_Aint const displacements[] = {
      offsetof(MPIDYNRES_pset, size),
      offsetof(MPIDYNRES_pset, _cap),
      offsetof(MPIDYNRES_pset, cr_ids),
  };
  static MPI_Datatype const types[] = {
      my_MPI_SIZE_T,
      my_MPI_SIZE_T,
      MPI_INT,
  };

  int const lengths[] = {
      1,    // one size_t
      1,    // one size_t
      cap,  // cap ints
  };

  assert(COUNT_OF(lengths) == COUNT_OF(displacements) &&
         COUNT_OF(displacements) == COUNT_OF(types));

  MPI_Type_create_struct(COUNT_OF(lengths), lengths, displacements, types,
                         &result);
  MPI_Type_commit(&result);
  return result;
}

/**
 * @brief      Get mpi datatype that can send an MPIDYNRES_uri_op_msg struct
 *
 * @details    free_all_mpi_datatypes has to be called when this function was
 * used
 *
 * @return     mpi datatype that can send an MPIDYNRES_uri_op_msg struct
 */
MPI_Datatype get_uri_op_datatype() {
  static MPI_Datatype result = NULL;

  static int const lengths[] = {
      MPI_MAX_PSET_NAME_LEN, MPI_MAX_PSET_NAME_LEN,
      1,  // one enum
  };
  static MPI_Aint const displacements[] = {
      offsetof(MPIDYNRES_URI_op_msg, uri1),
      offsetof(MPIDYNRES_URI_op_msg, uri2),
      offsetof(MPIDYNRES_URI_op_msg, op),
  };
  static MPI_Datatype const types[] = {
      MPI_CHAR,
      MPI_CHAR,
      MPI_INT,
  };

  if (result == NULL) {
    assert(COUNT_OF(lengths) == COUNT_OF(displacements) &&
           COUNT_OF(displacements) == COUNT_OF(types));

    MPI_Type_create_struct(COUNT_OF(lengths), lengths, displacements, types,
                           &result);
    MPI_Type_commit(&result);
  }
  return result;
}

/**
 * @brief      Get mpi datatype that can send an MPIDYNRES_rc_msg struct
 *
 * @details    free_all_mpi_datatypes has to be called when this function was
 * used
 *
 * @return     mpi datatype that can send an MPIDYNRES_rc_msg struct
 */
MPI_Datatype get_rc_datatype() {
  static MPI_Datatype result = NULL;

  static int const lengths[] = {
      1,  // one enum
      1,
      MPI_MAX_PSET_NAME_LEN,
  };
  static MPI_Aint const displacements[] = {
      offsetof(MPIDYNRES_RC_msg, type),
      offsetof(MPIDYNRES_RC_msg, tag),
      offsetof(MPIDYNRES_RC_msg, uri),
  };
  static MPI_Datatype const types[] = {
      MPI_INT,
      MPI_INT,
      MPI_CHAR,
  };

  if (result == NULL) {
    assert(COUNT_OF(lengths) == COUNT_OF(displacements) &&
           COUNT_OF(displacements) == COUNT_OF(types));

    MPI_Type_create_struct(COUNT_OF(lengths), lengths, displacements, types,
                           &result);
    MPI_Type_commit(&result);
  }
  return result;
}

/**
 * @brief      Get mpi datatype that can send an MPIDYNRES_rc_accept_msg struct
 *
 * @details    free_all_mpi_datatypes has to be called when this function was
 * used
 *
 * @return     mpi datatype that can send an MPIDYNRES_rc_accept_msg struct
 */
MPI_Datatype get_rc_accept_datatype() {
  static MPI_Datatype result = NULL;

  static int const lengths[] = {
      1,  // tag
      1,  // tag
      MPI_MAX_PSET_NAME_LEN,
  };
  static MPI_Aint const displacements[] = {
      offsetof(MPIDYNRES_RC_accept_msg, rc_tag),
      offsetof(MPIDYNRES_RC_accept_msg, new_process_tag),
      offsetof(MPIDYNRES_RC_accept_msg, uri),
  };
  static MPI_Datatype const types[] = {
      MPI_INT,
      MPI_INT,
      MPI_CHAR,
  };

  if (result == NULL) {
    assert(COUNT_OF(lengths) == COUNT_OF(displacements) &&
           COUNT_OF(displacements) == COUNT_OF(types));

    MPI_Type_create_struct(COUNT_OF(lengths), lengths, displacements, types,
                           &result);
    MPI_Type_commit(&result);
  }
  return result;
}

/**
 * @brief      Get mpi datatype that can send an MPIDYNRES_init_info_msg struct
 *
 * @details    free_all_mpi_datatypes has to be called when this function was
 * used
 *
 * @return     mpi datatype that can send an MPIDYNRES_init_info_msg struct
 */
MPI_Datatype get_init_info_datatype() {
  static MPI_Datatype result = NULL;

  static int const lengths[] = {
      MPI_MAX_PSET_NAME_LEN, MPI_MAX_PSET_NAME_LEN,
      1,  // tag
      1,  // tag
  };
  static MPI_Aint const displacements[] = {
      offsetof(MPIDYNRES_init_info_msg, uri_init),
      offsetof(MPIDYNRES_init_info_msg, uri_passed),
      offsetof(MPIDYNRES_init_info_msg, init_tag),
      offsetof(MPIDYNRES_init_info_msg, cr_id),
  };
  static MPI_Datatype const types[] = {
      MPI_CHAR,
      MPI_CHAR,
      MPI_INT,
      MPI_INT,
  };

  if (result == NULL) {
    assert(COUNT_OF(lengths) == COUNT_OF(displacements) &&
           COUNT_OF(displacements) == COUNT_OF(types));

    MPI_Type_create_struct(COUNT_OF(lengths), lengths, displacements, types,
                           &result);
    MPI_Type_commit(&result);
  }
  return result;
}
