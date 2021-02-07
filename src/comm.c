#include "comm.h"

#include <mpi.h>
#include <string.h>

#include "datastructures/mpidynres_pset.h"
#include "mpidynres.h"
#include "util.h"

int MPIDYNRES_Send_MPI_Info(MPI_Info info, int dest, int tag1, int tag2,
                            MPI_Comm comm) {
  int res;
  int nkeys;
  int vlen;
  int unused;
  char key[MPI_MAX_INFO_KEY + 1];
  struct info_serialized *serialized;

  if (info == MPI_INFO_NULL) {
    size_t bufsize = SIZE_MAX;
    res = MPI_Send(&bufsize, 1, my_MPI_SIZE_T, dest, tag1, comm);
    if (res) {
      return res;
    }
    return 0;
  }

  res = MPI_Info_get_nkeys(info, &nkeys);
  if (res) {
    return res;
  }
  size_t bufsize =
      sizeof(struct info_serialized) + 2 * nkeys * sizeof(ptrdiff_t);

  for (int i = 0; i < nkeys; i++) {
    MPI_Info_get_nthkey(info, i, key);
    MPI_Info_get_valuelen(info, key, &vlen, &unused);
    bufsize += sizeof(char) * (strlen(key) + 1);
    bufsize += sizeof(char) * (vlen + 1);
  }
  /*uint8_t *buffer = calloc(bufsize, 1);TODO: why the hell does this lead to heap corruption???????*/ 
  uint8_t *buffer = calloc(10*bufsize, 1);
  if (!buffer) {
    die("Memory error!\n");
  }
  serialized = (struct info_serialized *)buffer;
  serialized->num_strings = 2 * (size_t)nkeys;

  size_t offset =
      sizeof(struct info_serialized) + 2 * nkeys * sizeof(ptrdiff_t);
  for (int i = 0; i < nkeys; i++) {
    uint8_t *keystr = buffer + offset;
    serialized->strings[2 * i] = keystr - buffer;
    MPI_Info_get_nthkey(info, i, (char *)keystr);
    offset += sizeof(char) * (strlen((char *)keystr) + 1);
    uint8_t *valstr = buffer + offset;
    serialized->strings[2 * i + 1] = valstr - buffer;
    MPI_Info_get_valuelen(info, (char *)keystr, &vlen, &unused);
    MPI_Info_get(info, (char *)keystr, vlen + 1, (char *)valstr, &unused);
    offset += sizeof(char) * (vlen + 1);
  }
  /*BREAK();*/

  res = MPI_Send(&bufsize, 1, my_MPI_SIZE_T, dest, tag1, comm);
  if (res) {
    printf("%d\n", res);
    printf("MPI_Send failed in mpidynres_send_info\n");
    free(buffer);
    return res;
  }
  /*BREAK();*/
  res = MPI_Send(buffer, bufsize, MPI_BYTE, dest, tag2, comm);
  free(buffer);
  if (res) {
    return res;
  }
  return 0;
}

int MPIDYNRES_Recv_MPI_Info(MPI_Info *info, int source, int tag1, int tag2,
                            MPI_Comm comm, MPI_Status *status1,
                            MPI_Status *status2) {
  int res;
  size_t bufsize;
  struct info_serialized *serialized;

  res = MPI_Recv(&bufsize, 1, my_MPI_SIZE_T, source, tag1, comm, status1);
  if (res) {
    return res;
  }
  if (bufsize == SIZE_MAX) {
    *info = MPI_INFO_NULL;
    return 0;
  }
  uint8_t *buf = calloc(bufsize, 1);
  if (!buf) {
    die("Memory error!\n");
  }
  res = MPI_Recv(buf, bufsize, MPI_BYTE, source, tag2, comm, status2);
  if (res) {
    free(buf);
    return res;
  }
  serialized = (struct info_serialized *)buf;
  res = MPI_Info_create(info);
  if (res) {
    free(buf);
    return res;
  }
  for (size_t i = 0; i < serialized->num_strings/2; i++) {
    char *key = (char *)(buf + serialized->strings[2 * i]);
    char *val = (char *)(buf + serialized->strings[2 * i + 1]);
    res = MPI_Info_set(*info, key, val);
    if (res) {
      free(buf);
      return res;
    }
  }
  free(buf);
  return 0;
}

/**
 * @brief      Free MPI datatypes used for communication
 *
 * @details    This function will free all mpi datatypes that were created when
 * calling get_<datatype_name>_datatype() except for the pset
 */
void free_all_mpi_datatypes() {
  // pset type is freed in the applicaion
  MPI_Datatype types[] = {
      get_idle_command_datatype(),
      get_pset_op_datatype(),
      get_pset_free_datatype(),
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
 * @brief      Get mpi datatype that can send an MPIDYNRES_pset struct with
 * capacity cap
 *
 * @details    The datatype returned by this function hass to be free using
 * MPI_Type_free
 *
 * @param cap  The capacity of the pset
 *
 * @return     mpi datatype that can send an MPIDYNRES_uri_pset struct with
 * capacity cap
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
MPI_Datatype get_pset_op_datatype() {
  static MPI_Datatype result = NULL;

  static int const lengths[] = {
      MPI_MAX_PSET_NAME_LEN, MPI_MAX_PSET_NAME_LEN,
      1,  // one enum
  };
  static MPI_Aint const displacements[] = {
      offsetof(MPIDYNRES_pset_op_msg, uri1),
      offsetof(MPIDYNRES_pset_op_msg, uri2),
      offsetof(MPIDYNRES_pset_op_msg, op),
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

MPI_Datatype get_pset_free_datatype() {
  static MPI_Datatype result = NULL;

  static int const lengths[] = {
      1,  // session_id
      MPI_MAX_PSET_NAME_LEN,
  };
  static MPI_Aint const displacements[] = {
      offsetof(MPIDYNRES_pset_free_msg, session_id),
      offsetof(MPIDYNRES_pset_free_msg, pset_name),
  };
  static MPI_Datatype const types[] = {
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
