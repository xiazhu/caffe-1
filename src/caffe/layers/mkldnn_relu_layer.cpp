#ifdef MKLDNN_SUPPORTED
#include <algorithm>
#include <vector>

#include "caffe/layers/mkldnn_layers.hpp"

namespace caffe {

template <typename Dtype>
void MKLDNNReLULayer<Dtype>::LayerSetUp(const vector<Blob<Dtype>*>& bottom
                                        ,const vector<Blob<Dtype>*>& top)
{
    VLOG(1) << "MKLDNNReLULayer<Dtype>::LayerSetUp: " << this->layer_param_.name();

    NeuronLayer<Dtype>::LayerSetUp(bottom, top);
}

template <typename Dtype>
void MKLDNNReLULayer<Dtype>::Reshape(const vector<Blob<Dtype>*>& bottom
                                    ,const vector<Blob<Dtype>*>& top)
{
    VLOG(1) << "MKLDNNReLULayer<Dtype>::Reshape: " << this->layer_param_.name();

    NeuronLayer<Dtype>::Reshape(bottom, top);

    this->width_ = bottom[0]->width();
    this->height_ = bottom[0]->height();
    this->num_ = bottom[0]->num();
    this->channels_ = bottom[0]->channels();

}



template <typename Dtype>
void MKLDNNReLULayer<Dtype>::InitReLU(const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top)
{
    if (std::is_same<Dtype, double>::value) NOT_IMPLEMENTED;
    int32_t n  = this->num_;
    int32_t iw = this->width_;
    int32_t ih = this->height_;
    int32_t ic = this->channels_;

    Dtype negative_slope = this->layer_param_.relu_param().negative_slope();
    bool bottom_data_is_prv = (const_cast<Dtype*>(bottom[0]->prv_data()) != NULL);

    engine cpu_engine = CpuEngine::Instance().get_engine();
    memory::precision mpcsn = memory::precision::f32;
    // ---- Initialize memory descriptors -------------
    shared_ptr<memory::desc> input_md, output_md;
    shared_ptr<memory::primitive_desc> usr_mpd(NULL), prv_mpd(NULL);
    if (bottom_data_is_prv) {
        shared_ptr<MKLDNNMemoryDescriptor<Dtype, false> > mem_descr
            = get_mkldnn_prv_descriptor<Dtype, false>(bottom[0]);
        input_md.reset(new memory::desc(mem_descr->prv_memory_pd()->desc()));
        usr_mpd = mem_descr->usr_memory_pd();
        prv_mpd = mem_descr->prv_memory_pd();
    } else {
        input_md.reset(new memory::desc({{n, ic, ih, iw}}, mpcsn, memory::format::nchw));
        usr_mpd.reset(new memory::primitive_desc(*input_md, cpu_engine));
    }
    output_md = input_md;

    // ---- Initialize relu primitive descriptor -------------
    relu::desc reluFwd_desc(prop_kind::forward, negative_slope, *input_md, *output_md);
    reluFwd_pd.reset(new relu::primitive_desc(reluFwd_desc, cpu_engine));

    // ---  init primitive and prv_memory descriptors ----------------------
    fwd_bottom_data.reset(new MKLDNNData<Dtype>(usr_mpd, prv_mpd, bottom[0], this));
    input_primitive = fwd_bottom_data->create_input(false);

    fwd_top_data.reset(new MKLDNNData<Dtype>(usr_mpd, prv_mpd, top[0], this));
    output_memory = fwd_top_data->create_output_memory();

    reluFwd.reset(new relu(*reluFwd_pd, *input_primitive, *output_memory));
    fwd_bottom_data->set_mkldnn_primitive(reluFwd);
    fwd_top_data->set_mkldnn_primitive(reluFwd);
}


template <typename Dtype>
void MKLDNNReLULayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom
                                        ,const vector<Blob<Dtype>*>& top)
{
    VLOG(1) << "MKLDNNReLULayer<Dtype>::Forward_cpu: " << this->layer_param_.name();
    if( reluFwd_pd == NULL)
        InitReLU(bottom, top);
    // making reorders if needed.
    fwd_bottom_data->sync_before_read(false);
    // update top that head at prv
    fwd_top_data->sync_before_write();

    reluFwd.submit();
}

template <typename Dtype>
void MKLDNNReLULayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top
                                            ,const vector<bool>& propagate_down
                                            ,const vector<Blob<Dtype>*>& bottom)
{ NOT_IMPLEMENTED; }

#ifdef CPU_ONLY
STUB_GPU(MKLDNNReLULayer);
#else
template <typename Dtype>
void MKLDNNReLULayer<Dtype>::Forward_gpu(const vector<Blob<Dtype>*>& bottom
                                        ,const vector<Blob<Dtype>*>& top)
{ NOT_IMPLEMENTED; }

template <typename Dtype>
void MKLDNNReLULayer<Dtype>::Backward_gpu(const vector<Blob<Dtype>*>& top
                                            ,const vector<bool>& propagate_down
                                            ,const vector<Blob<Dtype>*>& bottom)
{ NOT_IMPLEMENTED; }
#endif

INSTANTIATE_CLASS(MKLDNNReLULayer);
}  // namespace caffe
#endif  // #ifdef MKLDNN_SUPPORTED
