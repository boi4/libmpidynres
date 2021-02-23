
int MPIDYNRES_manager_get_initial_pset(MPIDYNRES_manager manager,
                                       set_int *o_initial_pset) {
  inc_dec_manager *mgr = (inc_dec_mgr *)manager;
  int vlen;
  bool in_there;
  int num_init = 1;
  MPI_Info config = mgr->scheduler->config->manager_config;

  MPI_Info_get_valuelen(config, "manager_initial_number", &vlen, &in_there);
  if (in_there) {
    char *value = calloc(vlen+1, sizeof(char));
    MPI_Info_get(config, "manager_initial_number", vlen, value, &in_there);
    num_init = atoi(value);
    free(value);
    if (num_init < 1 || num_init > mgr->num_processes) {
      die("Key manager_initial_number is invalid.\n");
    }
  } else {
    MPI_Info_get_valuelen(config, "manager_initial_number_random", &vlen, &in_there);
    if (in_there) {
      num_init = 1 + (rand() % (mgr->num_processes - 1));
    } else {
      num_init = 1;
    }
  }

  *o_initial_pset = pset_init(int_compare);
  for (int i = 0; i < num_init; i++) {
    pset_init_insert(o_initial_pset, );
  }

  return 0;
}


  /*double sqrt_2_pi = 0.7978845608028654;  // sqrt(2/pi)*/

  /*int s;*/
  /*MPI_Comm_size(MPI_COMM_WORLD, &s);*/
  /*double a = s / sqrt_2_pi / 4.0;*/

  /**o_config =*/
      /*(MPIDYNRESSIM_config){.base_communicator = MPI_COMM_WORLD,*/
                         /*.num_init_crs = (s - 1) / 2 == 0 ? 1 : (s - 1) / 2,*/
                         /*.scheduling_mode = MPIDYNRES_MODE_RANDOM_DIFF,*/
                         /*.random_diff_conf =*/
                             /*{*/
                                 /*.std_dev = a,*/
                             /*}*/

      /*};*/
