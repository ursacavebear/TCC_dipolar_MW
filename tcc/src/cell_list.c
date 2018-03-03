#include "cell_list.h"
#include "voronoi_bonds.h"
#include "globals.h"
#include "tools.h"
#include "bonds.h"

void Get_Bonds_With_Voronoi_And_Cell_List() {  // Get bonds using Voronoi
    int i, j;
    int ic, jcell0, jcell,nabor;    // various counters
    const int nBs = 4 * nB;
    int num_particle_1_neighbours;
    int *particle_1_neighbours, *sorted_particle_1_neighbours, *particle_1_bonds;
    double *particle_1_bond_lengths, *sorted_particle_1_bond_lengths;
    double squared_distance;

    double *store_dr2;
    int *temp_cnb, **temp_bNums;
    char errMsg[1000];


    particle_1_neighbours = malloc(nBs*sizeof(int));
    sorted_particle_1_neighbours = malloc(nBs*sizeof(int));
    particle_1_bonds = malloc(nBs*sizeof(int));
    particle_1_bond_lengths = malloc(nBs*sizeof(double));
    sorted_particle_1_bond_lengths = malloc(nBs*sizeof(double));

    store_dr2 = malloc(current_frame_particle_number*sizeof(double));   if (store_dr2==NULL) { sprintf(errMsg,"Get_Bonds_With_Voronoi_And_Cell_List(): store_dr2[] malloc out of memory\n");    Error(errMsg); }
    temp_cnb = malloc(current_frame_particle_number*sizeof(int));   if (temp_cnb==NULL) { sprintf(errMsg,"Get_Bonds_With_Voronoi_And_Cell_List(): temp_cnb[] malloc out of memory\n");  Error(errMsg); }
    temp_bNums = malloc(current_frame_particle_number*sizeof(int *));   if (temp_bNums==NULL) { sprintf(errMsg,"Get_Bonds_With_Voronoi_And_Cell_List(): temp_bNums[] malloc out of memory\n");  Error_no_free(errMsg); }
    for (j=0; j<current_frame_particle_number; ++j) {
        temp_bNums[j] = malloc(nBs*sizeof(int));
        if (temp_bNums[j]==NULL) {
            sprintf(errMsg,"Get_Bonds_With_Voronoi_And_Cell_List(): temp_bNums[][] malloc out of memory\n"); Error_no_free(errMsg);
        }
    }

    printf("Vor: N%d rcut2 %.15lg\n",current_frame_particle_number,rcutAA2);

    if (PRINTINFO==1) {
        printf("Voronoi fc %lg rcutAA %lg\n",fc,rcutAA);
        if (PBCs==0) printf("No bonds through edge of box\n\n");
        else  printf("Periodic Boundary Conditions - PBC bonds\n\n");
    }
    llist[0]=-1;
    for (i=0; i<current_frame_particle_number; ++i) {
        llist[i+1]=-1;
        cnb[i] = 0;
        temp_cnb[i]=0;
        store_dr2[i]=-1.0;
        for (j=0; j<nBs; j++) temp_bNums[i][j]=0;
    }
    links();
    for (ic=1;ic<=n_cells_total;ic++) {        // loop over all cells
        i=head[ic];     // head of list particle for cell ic
        while (i>0) {   // loop over all particles in ic

            j=llist[i]; // next particle in current cell ic
            while (j>0) {   // loop over all particles in cell ic
                squared_distance = Get_Interparticle_Distance(i - 1, j - 1);
                if (squared_distance < rcutAA2) {
                    if (temp_cnb[i-1] < nBs && temp_cnb[j-1] < nBs) {  // max number of bonds, do ith particle
                        temp_bNums[i-1][temp_cnb[i-1]]=j-1;
                        temp_bNums[j-1][temp_cnb[j-1]]=i-1;
                        temp_cnb[i-1]++;
                        temp_cnb[j-1]++;
                    }
                    else {    // list is now full
                        Too_Many_Bonds(i-1, j-1, __func__);
                    }
                }
                j=llist[j]; // loop over next particle in cell ic
            }
            jcell0=13*(ic-1);       // now loop over adjacent cells to cell ic
            for (nabor=1;nabor<=13;nabor++) {
                jcell=map[jcell0+nabor];
                j=head[jcell];  // head of cell for jcell
                while (j>0) {   // loop over head of cell and all other particles in jcell
                    squared_distance = Get_Interparticle_Distance(i - 1, j - 1);
                    if (squared_distance < rcutAA2) {
                        if (temp_cnb[i-1] < nBs && temp_cnb[j-1] < nBs) {  // max number of bonds, do ith particle
                            temp_bNums[i-1][temp_cnb[i-1]]=j-1;
                            temp_bNums[j-1][temp_cnb[j-1]]=i-1;
                            temp_cnb[i-1]++;
                            temp_cnb[j-1]++;
                        }
                        else {
                            Too_Many_Bonds(i-1, j-1, __func__);
                        }
                    }
                    j=llist[j]; // next particle in jcell
                }
            }
            i=llist[i]; // next particle in ic cell
        }
    }

    for (i=0; i<current_frame_particle_number; ++i) {
        num_particle_1_neighbours = 0;
        for (j=0; j<temp_cnb[i]; ++j) {
            squared_distance = Get_Interparticle_Distance(i, temp_bNums[i][j]);
            particle_1_neighbours[num_particle_1_neighbours] = temp_bNums[i][j];
            particle_1_bonds[num_particle_1_neighbours] = 1;
            particle_1_bond_lengths[num_particle_1_neighbours] = squared_distance;
            store_dr2[temp_bNums[i][j]]=squared_distance;
            num_particle_1_neighbours++;
        }
        Insertion_Sort_Bond_Lengths(num_particle_1_neighbours, particle_1_neighbours, sorted_particle_1_neighbours, particle_1_bond_lengths, sorted_particle_1_bond_lengths);

        for (j=0; j<num_particle_1_neighbours; ++j) {
            particle_1_bonds[j] = 1;
        }

        Remove_Unbonded_Neighbours(i, num_particle_1_neighbours, sorted_particle_1_neighbours, particle_1_bonds);

        check_bond_cut_offs(i, num_particle_1_neighbours, sorted_particle_1_neighbours, sorted_particle_1_bond_lengths, particle_1_bonds);

        add_new_voronoi_bond(i, num_particle_1_neighbours, sorted_particle_1_neighbours, store_dr2, particle_1_bonds);
    }

    for (i=0; i<current_frame_particle_number; i++) free(temp_bNums[i]);
    free(temp_bNums);
    free(temp_cnb);
    free(store_dr2);
    free(particle_1_neighbours);
    free(sorted_particle_1_neighbours);
    free(particle_1_bonds);
    free(particle_1_bond_lengths);
    free(sorted_particle_1_bond_lengths);
}

void links() {  // sorts all the particles into cells, result given by head-of-chain and linked list arrays
    int i, ic;
    for (ic=1;ic<=n_cells_total;ic++) head[ic]=0;
    for (i=1;i<=current_frame_particle_number;i++) {
        ic = 1 + (int)((x[i-1]+ half_sidex)*inv_cell_len_x)
             + n_cells_y*((int)((y[i-1]+half_sidex)*inv_cell_len_y))
             + n_cells_z*n_cells_z*((int)((z[i-1]+half_sidex)*inv_cell_len_z));
        if (ic > n_cells_total || ic <= 0) {
            printf("i %d r_x %lg r_y %lg r_z %lg side %lg halfSide %lg ic %d ncells %d\n",i-1,x[i-1],y[i-1],z[i-1],sidex,half_sidex,ic,n_cells_total);
            Error("links(): ic > ncells, i.e. particle coord no longer in simulation box!!\n");
        }
        llist[i]=head[ic];
        head[ic]=i;
    }
}

int icell(int tix, int tiy, int tiz) { 	// returns cell number (from 1 to ncells) for given (tix,tiy,tiz) coordinate
    return 1 + (tix - 1 + n_cells_x) % n_cells_x
           + n_cells_y * ((tiy - 1 + n_cells_y) % n_cells_y)
           + n_cells_z * n_cells_z * ((tiz - 1 + n_cells_z) % n_cells_z);
}

void Setup_Cell_List() {

    int i;
    int ix, iy, iz;
    int imap;
    char errMsg[1000];

    n_cells_x = (int)(sidex/rcutAA);
    n_cells_y = (int)(sidey/rcutAA);
    n_cells_z = (int)(sidez/rcutAA);
    if (n_cells_x < 3 || n_cells_y < 3 || n_cells_z < 3) Error_no_free("main(): M<3, too few cells");
    n_cells_total = n_cells_x*n_cells_y*n_cells_z;
    inv_cell_len_x = n_cells_x/sidex;
    inv_cell_len_y = n_cells_y/sidey;
    inv_cell_len_z = n_cells_z/sidez;

    printf("x_cells %d y_cells %d z_cells %d total_cells %d\n", n_cells_x, n_cells_y, n_cells_z, n_cells_total);

    head=malloc((n_cells_total+1)*sizeof(int));    if (head==NULL) { sprintf(errMsg,"Initialise_Global_Variables(): head[] malloc out of memory\n");  Error_no_free(errMsg); }
    map=malloc((13*n_cells_total+1)*sizeof(int));  if (map==NULL) { sprintf(errMsg,"Initialise_Global_Variables(): map[] malloc out of memory\n");    Error_no_free(errMsg); }
    llist=malloc((current_frame_particle_number+1)*sizeof(int)); if (llist==NULL) { sprintf(errMsg,"Initialise_Global_Variables(): llist[] malloc out of memory\n");    Error_no_free(errMsg); }

    for (i=0; i<n_cells_total+1; i++) head[i]=0;
    for (i=0; i<13*n_cells_total+1; i++) map[i]=0;
    for (i=0; i<current_frame_particle_number+1; i++) llist[i]=0;


    // routine to create the thirteen nearest neighbours array map[] of each cell
    for (iz=1; iz<=n_cells_z; iz++) {
        for (iy=1; iy<=n_cells_y; iy++) {
            for (ix=1; ix<=n_cells_x; ix++) {
                imap = (icell(ix,iy,iz)-1)*13;
                map[imap+1 ]=icell(ix+1, iy,   iz);
                map[imap+2 ]=icell(ix+1, iy+1, iz);
                map[imap+3 ]=icell(ix,   iy+1, iz);
                map[imap+4 ]=icell(ix-1, iy+1, iz);
                map[imap+5 ]=icell(ix+1, iy,   iz-1);
                map[imap+6 ]=icell(ix+1, iy+1, iz-1);
                map[imap+7 ]=icell(ix,   iy+1, iz-1);
                map[imap+8 ]=icell(ix-1, iy+1, iz-1);
                map[imap+9 ]=icell(ix+1, iy,   iz+1);
                map[imap+10]=icell(ix+1, iy+1, iz+1);
                map[imap+11]=icell(ix,   iy+1, iz+1);
                map[imap+12]=icell(ix-1, iy+1, iz+1);
                map[imap+13]=icell(ix,   iy,   iz+1);
            }
        }
    }
}