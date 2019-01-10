#ifndef TCC_11F_H
#define TCC_11F_H

void Clusters_Get11F_13K();

int are_spindles_bonded(int first_5A_id, int second_5A_id, int *bonded_pairs);

int count_bonded_ring_particles_11F(const int *first_5A, const int *second_5A, int *bonded_particle_ids);

int get_bonded_6As(int *bonded_6A_id, int *bonded_pairs);

void write_11F(const int *trial_cluster, const int *first_5A, const int *second_5A);

#endif
