#pragma once

#include <boost/serialization/serialization.hpp>

#include "lbl/metadata.h"
#include "lbl/utils.h"

namespace oxlm {

typedef Eigen::Map<MatrixReal> ContextTransformType;
typedef vector<ContextTransformType> ContextTransformsType;
typedef Eigen::Map<MatrixReal> WordVectorsType;
typedef Eigen::Map<VectorReal> WeightsType;

class Weights {
 public:
  Weights();

  Weights(
      const ModelData& config,
      const boost::shared_ptr<Metadata>& metadata,
      bool model_weights = false);

  void getContextVectors(
      const boost::shared_ptr<Corpus>& corpus,
      const vector<int>& indices,
      vector<vector<int>>& contexts,
      vector<MatrixReal>& context_vectors) const;

  MatrixReal getPredictionVectors(
      const vector<int>& indices,
      const vector<MatrixReal>& context_vectors) const;

  MatrixReal getContextProduct(
      int index, const MatrixReal& representations) const;

  void getGradient(
      const boost::shared_ptr<Corpus>& corpus,
      const vector<int>& indices,
      const vector<vector<int>>& contexts,
      const vector<MatrixReal>& context_vectors,
      const MatrixReal& prediction_vectors,
      boost::shared_ptr<Weights>& gradient,
      Real& objective) const;

  void getContextGradient(
      const vector<int>& indices,
      const vector<vector<int>>& contexts,
      const vector<MatrixReal>& context_vectors,
      const MatrixReal& weighted_representations,
      boost::shared_ptr<Weights>& gradient) const;

  void update(const boost::shared_ptr<Weights>& gradient);

  void updateSquared(const boost::shared_ptr<Weights>& global_gradient);

  void updateAdaGrad(
      const boost::shared_ptr<Weights>& global_gradient,
      const boost::shared_ptr<Weights>& adagrad);

  Real regularizerUpdate(Real minibatch_factor);

  Real predict(int word_id, const vector<int>& context) const;

  void clear();

  virtual ~Weights();

 private:
  friend class boost::serialization::access;

  template<class Archive>
  void save(Archive& ar, const unsigned int version) const {
    ar << config;
    ar << metadata;
    ar << size;
    ar << boost::serialization::make_array(data, size);
  }

  template<class Archive>
  void load(Archive& ar, const unsigned int version) {
    ar >> config;
    ar >> metadata;
    ar >> size;
    data = new Real[size];
    ar >> boost::serialization::make_array(data, size);
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER();

 protected:
  ModelData config;
  boost::shared_ptr<Metadata> metadata;

  int size;
  Real* data;

  ContextTransformsType C;
  WordVectorsType       Q;
  WordVectorsType       R;
  WeightsType           B;
  WeightsType           W;
};

} // namespace oxlm
