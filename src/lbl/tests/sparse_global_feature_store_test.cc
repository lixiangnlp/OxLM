#include "gtest/gtest.h"

#include <sstream>
#include <vector>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include "lbl/class_context_extractor.h"
#include "lbl/context_processor.h"
#include "lbl/sparse_global_feature_store.h"
#include "lbl/sparse_minibatch_feature_store.h"
#include "lbl/word_to_class_index.h"
#include "utils/constants.h"
#include "utils/testing.h"

using namespace std;
namespace ar = boost::archive;

namespace oxlm {

class SparseGlobalFeatureStoreTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    vector<int> data = {2, 3, 4, 5, 6};
    vector<int> classes = {0, 2, 7};
    boost::shared_ptr<Corpus> corpus = boost::make_shared<Corpus>(data);
    boost::shared_ptr<WordToClassIndex> index =
        boost::make_shared<WordToClassIndex>(classes);
    boost::shared_ptr<ContextProcessor> processor =
        boost::make_shared<ContextProcessor>(corpus, 1);
    boost::shared_ptr<FeatureContextHasher> hasher =
        boost::make_shared<FeatureContextHasher>(corpus, index, processor, 1);
    boost::shared_ptr<FeatureContextExtractor> extractor =
        boost::make_shared<ClassContextExtractor>(hasher);

    store = SparseGlobalFeatureStore(5, 4, extractor);
    SparseMinibatchFeatureStore g_store(5, extractor);

    VectorReal values(5);

    context1 = {2};
    int feature_context_id = 1;
    values = SparseVectorReal(5);
    store.hintFeatureIndex(feature_context_id, 0);
    store.hintFeatureIndex(feature_context_id, 1);

    values = SparseVectorReal(5);
    values << 5, 3, 0, 0, 0;
    g_store.hintFeatureIndex(feature_context_id, 0);
    g_store.hintFeatureIndex(feature_context_id, 1);
    g_store.update(context1, values);

    context2 = {3};
    feature_context_id = 2;
    store.hintFeatureIndex(feature_context_id, 2);
    store.hintFeatureIndex(feature_context_id, 3);

    values = SparseVectorReal(5);
    values << 0, 0, 2, 1, 0;
    g_store.hintFeatureIndex(feature_context_id, 2);
    g_store.hintFeatureIndex(feature_context_id, 3);
    g_store.update(context2, values);

    gradient_store = boost::make_shared<SparseMinibatchFeatureStore>(g_store);
  }

  SparseGlobalFeatureStore store;
  boost::shared_ptr<MinibatchFeatureStore> gradient_store;
  vector<int> context1, context2;
};

TEST_F(SparseGlobalFeatureStoreTest, TestUpdateSquared) {
  store.updateSquared(gradient_store);

  EXPECT_EQ(4, store.size());
  VectorReal expected_values(5);
  expected_values << 25, 9, 0, 0, 0;
  EXPECT_MATRIX_NEAR(expected_values, store.get(context1), EPS);
  expected_values << 0, 0, 4, 1, 0;
  EXPECT_MATRIX_NEAR(expected_values, store.get(context2), EPS);
}

TEST_F(SparseGlobalFeatureStoreTest, TestUpdateAdaGrad) {
  store.updateSquared(gradient_store);
  boost::shared_ptr<GlobalFeatureStore> adagrad_store =
      boost::make_shared<SparseGlobalFeatureStore>(store);

  store.updateAdaGrad(gradient_store, adagrad_store, 1);

  EXPECT_EQ(4, store.size());
  VectorReal expected_values(5);
  expected_values << 24, 8, 0, 0, 0;
  EXPECT_MATRIX_NEAR(expected_values, store.get(context1), EPS);
  expected_values << 0, 0, 3, 0, 0;
  EXPECT_MATRIX_NEAR(expected_values, store.get(context2), EPS);

}

TEST_F(SparseGlobalFeatureStoreTest, TestUpdateRegularizer) {
  store.updateSquared(gradient_store);
  store.l2GradientUpdate(gradient_store, 0.5);
  EXPECT_NEAR(180.75, store.l2Objective(gradient_store, 1), EPS);

  VectorReal expected_values(5);
  expected_values << 12.5, 4.5, 0, 0, 0;
  EXPECT_MATRIX_NEAR(expected_values, store.get(context1), EPS);
  expected_values << 0, 0, 2, 0.5, 0;
  EXPECT_MATRIX_NEAR(expected_values, store.get(context2), EPS);
}

TEST_F(SparseGlobalFeatureStoreTest, TestSerialization) {
  stringstream stream(ios_base::binary | ios_base::out | ios_base::in);
  ar::binary_oarchive output_stream(stream);
  output_stream << store;

  SparseGlobalFeatureStore store_copy;
  ar::binary_iarchive input_stream(stream);
  input_stream >> store_copy;

  EXPECT_EQ(store, store_copy);
}

}
