#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <vector>
#include "caffe/internode/tree_cluster.hpp"
#include "caffe/MultiSolver.hpp"


// Modified by Jian
#include <iostream>
#include <mpi.h>

namespace caffe {

template <typename Dtype>
MultiSolver<Dtype>::MultiSolver(shared_ptr<Solver<Dtype> > root_solver)
  : root_solver_(root_solver)
  , iter_size(root_solver->param().iter_size()) {
  root_solver->set_forward_backward(
    boost::bind(&MultiSolver<Dtype>::ForwardBackward, this));
}

template <typename Dtype>
Dtype MultiSolver<Dtype>::ForwardBackwardImpl(bool first, bool last) {
  Dtype loss = 0;
  Net<Dtype>& net = *root_solver_->net();
  for (int i = 0; i < net.layers().size(); ++i) {
    if (first) {
      for (int j = 0; j < callbacks_.size(); ++j) {
        callbacks_[j]->on_start(i);
      }
    }

    vector<int> param_ids = net.get_layer_learnable_param_ids(i);
    loss += root_solver_->net()->ForwardFromTo(i, i);

    if (last) {
      for (int j = 0; j < callbacks_.size(); ++j) {
        callbacks_[j]->on_forward_finished(i);
      }
    }

  }

  for (int i = net.layers().size() - 1; i >= 0; --i) {
    if (first) {
      for (int j = 0; j < callbacks_.size(); ++j) {
        callbacks_[j]->on_backward_start(i);
      }
    }
    root_solver_->net()->BackwardFromTo(i, i);

    // // DEBUG
    // if (i == 12) {
    //   int mpi_size;
    //   int param_server_rank;
    //   int mpi_rank;
    //   MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    //   MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    //   // Blob<Dtype>* blob = blob_accessor->get_blob(layer_id, 0);

    //   Dtype val = 0.0;
    //   // for (int i = 0; i < this->net().layers()[12]->blobs()[0]->count(); i++) {
    //     // val += this->net().layers()[12]->blobs()[0]->cpu_diff()[i];
    //     val += this->net().layers()[12]->blobs()[0]->cpu_diff()[0];
    //   // }


    //   LOG(INFO) << "blob 12 0 generated " << val << " mpi rank " << mpi_rank;

    // }



    if (last) {
      for (int j = 0; j < callbacks_.size(); ++j) {
        callbacks_[j]->on_gradients_ready(i);
      }
    }
  }
  return loss;
}

template <typename Dtype>
Dtype MultiSolver<Dtype>::ForwardBackward() {
  Dtype loss = 0;
  for (int i = 0; i < iter_size; ++i) {
    loss += ForwardBackwardImpl(
      (i == 0), (i + 1 == iter_size));
  }
  return loss / iter_size;
}

template <typename Dtype>
void MultiSolver<Dtype>::Solve() {
  root_solver_->Solve();
}

INSTANTIATE_CLASS(MultiSolver);

}  // namespace caffe

