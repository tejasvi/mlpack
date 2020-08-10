/**
  * @file cf_test.cpp
  * @author Wenhao Huang
  *
  * Test mlpackMain() of cf_main.cpp
  *
  * mlpack is free software; you may redistribute it and/or modify it under the
  * terms of the 3-clause BSD license.  You should have received a copy of the
  * 3-clause BSD license along with mlpack.  If not, see
  * http://www.opensource.org/licenses/BSD-3-Clause for more information.
  */
#include <string>

#define BINDING_TYPE BINDING_TYPE_TEST

static const std::string testName = "CollaborativeFiltering";

#include <mlpack/core.hpp>
#include <mlpack/methods/cf/cf_main.cpp>
#include <mlpack/core/util/mlpack_main.hpp>
#include <mlpack/core/math/random.hpp>
#include "test_helper.hpp"

#include <boost/test/unit_test.hpp>

using namespace mlpack;
using namespace arma;


struct CFTestFixture
{
 public:
  CFTestFixture()
  {
    // Cache in the options for this program.
    IO::RestoreSettings(testName);
  }

  ~CFTestFixture()
  {
    // Clear the settings.
    bindings::tests::CleanMemory();
    IO::ClearSettings();
  }
};

static void ResetSettings()
{
  bindings::tests::CleanMemory();
  IO::ClearSettings();
  IO::RestoreSettings(testName);
}

BOOST_FIXTURE_TEST_SUITE(CFMainTest, CFTestFixture);

/**
 * Ensure the rank is non-negative.
 */
BOOST_AUTO_TEST_CASE(CFRankBoundTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);

  // Rank should not be negative.
  SetInputParam("rank", int(-1));
  SetInputParam("training", std::move(dataset));

  Log::Fatal.ignoreInput = true;
  BOOST_REQUIRE_THROW(mlpackMain(), std::runtime_error);
  Log::Fatal.ignoreInput = false;
}

/**
 * Ensure min_residue is non-negative.
 */
BOOST_AUTO_TEST_CASE(CFMinResidueBoundTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);

  // min_residue should not be negative.
  SetInputParam("min_residue", double(-1));
  SetInputParam("training", std::move(dataset));

  Log::Fatal.ignoreInput = true;
  BOOST_REQUIRE_THROW(mlpackMain(), std::runtime_error);
  Log::Fatal.ignoreInput = false;
}

/**
 * Ensure max_iterations is non-negative.
 */
BOOST_AUTO_TEST_CASE(CFMaxIterationsBoundTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);

  // max_iterations should not be negative.
  SetInputParam("max_iterations", int(-1));
  SetInputParam("training", std::move(dataset));

  Log::Fatal.ignoreInput = true;
  BOOST_REQUIRE_THROW(mlpackMain(), std::runtime_error);
  Log::Fatal.ignoreInput = false;
}

/**
 * Ensure recommendations is positive.
 */
BOOST_AUTO_TEST_CASE(CFRecommendationsBoundTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);

  // recommendations should not be zero.
  SetInputParam("recommendations", int(0));
  SetInputParam("all_user_recommendations", true);
  SetInputParam("training", std::move(dataset));
  SetInputParam("max_iterations", int(5));

  Log::Fatal.ignoreInput = true;
  BOOST_REQUIRE_THROW(mlpackMain(), std::runtime_error);
  Log::Fatal.ignoreInput = false;

  // recommendations should not be negative.
  SetInputParam("recommendations", int(-1));

  Log::Fatal.ignoreInput = true;
  BOOST_REQUIRE_THROW(mlpackMain(), std::runtime_error);
  Log::Fatal.ignoreInput = false;
}

/**
 * Ensure neighborhood is positive and not larger than the number of users.
 */
BOOST_AUTO_TEST_CASE(CFNeighborhoodBoundTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);
  const size_t userNum = max(dataset.row(0)) + 1;

  // neighborhood should not be zero.
  SetInputParam("neighborhood", int(0));
  SetInputParam("training", std::move(dataset));

  Log::Fatal.ignoreInput = true;
  BOOST_REQUIRE_THROW(mlpackMain(), std::runtime_error);
  Log::Fatal.ignoreInput = false;

  // neighborhood should not be negative.
  SetInputParam("neighborhood", int(-1));

  Log::Fatal.ignoreInput = true;
  BOOST_REQUIRE_THROW(mlpackMain(), std::runtime_error);
  Log::Fatal.ignoreInput = false;

  // neighborhood should not be larger than the number of users.
  SetInputParam("neighborhood", int(userNum + 1));

  Log::Fatal.ignoreInput = true;
  BOOST_REQUIRE_THROW(mlpackMain(), std::runtime_error);
  Log::Fatal.ignoreInput = false;
}

/**
 * Ensure algorithm is one of { "NMF", "BatchSVD",
 * "SVDIncompleteIncremental", "SVDCompleteIncremental", "RegSVD",
 * "BiasSVD", "SVDPP" }.
 */
BOOST_AUTO_TEST_CASE(CFAlgorithmBoundTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);

  // algorithm should be valid.
  SetInputParam("algorithm", std::string("invalid_algorithm"));
  SetInputParam("training", std::move(dataset));

  Log::Fatal.ignoreInput = true;
  BOOST_REQUIRE_THROW(mlpackMain(), std::runtime_error);
  Log::Fatal.ignoreInput = false;
}

/**
 * Ensure saved models can be reused again.
 */
BOOST_AUTO_TEST_CASE(CFModelReuseTest)
{
  std::string algorithms[] = { "NMF", "BatchSVD",
      "SVDIncompleteIncremental", "SVDCompleteIncremental", "RegSVD",
      "BiasSVD", "SVDPP" };

  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);

  for (std::string& algorithm : algorithms)
  {
    ResetSettings();
    SetInputParam("training", dataset);
    SetInputParam("max_iterations", int(10));
    SetInputParam("algorithm", algorithm);

    mlpackMain();

    // Reset passed parameters.
    IO::GetSingleton().Parameters()["training"].wasPassed = false;
    IO::GetSingleton().Parameters()["max_iterations"].wasPassed = false;
    IO::GetSingleton().Parameters()["algorithm"].wasPassed = false;

    // Reuse the model to get recommendations.
    int recommendations = 3;
    const int querySize = 7;
    Mat<size_t> query =
        arma::linspace<Mat<size_t>>(0, querySize - 1, querySize);

    SetInputParam("query", std::move(query));
    SetInputParam("recommendations", recommendations);
    SetInputParam("input_model",
        std::move(IO::GetParam<CFModel*>("output_model")));

    mlpackMain();

    const Mat<size_t>& output = IO::GetParam<Mat<size_t>>("output");

    BOOST_REQUIRE_EQUAL(output.n_rows, recommendations);
    BOOST_REQUIRE_EQUAL(output.n_cols, querySize);
  }
}

/**
 * Ensure output of all_user_recommendations has correct size.
 */
BOOST_AUTO_TEST_CASE(CFAllUserRecommendationsTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);
  const size_t userNum = max(dataset.row(0)) + 1;

  SetInputParam("training", std::move(dataset));
  SetInputParam("max_iterations", int(10));
  SetInputParam("all_user_recommendations", true);

  mlpackMain();

  const Mat<size_t>& output = IO::GetParam<Mat<size_t>>("output");

  BOOST_REQUIRE_EQUAL(output.n_cols, userNum);
}

/**
 * Test that rank is used.
 */
BOOST_AUTO_TEST_CASE(CFRankTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);
  int rank = 7;

  SetInputParam("training", std::move(dataset));
  SetInputParam("rank", rank);
  SetInputParam("max_iterations", int(10));
  SetInputParam("algorithm", std::string("NMF"));

  mlpackMain();

  const CFModel* outputModel = IO::GetParam<CFModel*>("output_model");

  BOOST_REQUIRE_EQUAL(outputModel->template CFPtr<NMFPolicy>()->Rank(), rank);
}

/**
 * Test that min_residue is used.
 */
BOOST_AUTO_TEST_CASE(CFMinResidueTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);
  const CFModel* outputModel;

  // Set a larger min_residue.
  SetInputParam("min_residue", double(100));
  SetInputParam("training", dataset);
  // Remove the influence of max_iterations.
  SetInputParam("max_iterations", int(1e4));

  // The execution of CF algorithm depends on initial random seed.
  mlpack::math::FixedRandomSeed();
  mlpackMain();

  outputModel =  IO::GetParam<CFModel*>("output_model");
  // By default the main program use NMFPolicy.
  const mat w1 = outputModel->template CFPtr<NMFPolicy>()->Decomposition().W();
  const mat h1 = outputModel->template CFPtr<NMFPolicy>()->Decomposition().H();

  ResetSettings();

  // Set a smaller min_residue.
  SetInputParam("min_residue", double(0.1));
  SetInputParam("training", std::move(dataset));
  // Remove the influence of max_iterations.
  SetInputParam("max_iterations", int(1e4));

  // The execution of CF algorithm depends on initial random seed.
  mlpack::math::FixedRandomSeed();
  mlpackMain();

  outputModel = IO::GetParam<CFModel*>("output_model");
  // By default the main program use NMFPolicy.
  const mat w2 = outputModel->template CFPtr<NMFPolicy>()->Decomposition().W();
  const mat h2 = outputModel->template CFPtr<NMFPolicy>()->Decomposition().H();

  // The resulting matrices should be different.
  BOOST_REQUIRE(arma::norm(w1 - w2) > 1e-5 || arma::norm(h1 - h2) > 1e-5);
}

/**
 * Test that itertaion_only_termination is used.
 */
BOOST_AUTO_TEST_CASE(CFIterationOnlyTerminationTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);
  const CFModel* outputModel;

  // Set iteration_only_termination.
  SetInputParam("iteration_only_termination", true);
  SetInputParam("training", dataset);
  SetInputParam("max_iterations", int(100));
  SetInputParam("min_residue", double(1e9));

  // The execution of CF algorithm depends on initial random seed.
  mlpack::math::FixedRandomSeed();
  mlpackMain();

  outputModel =  IO::GetParam<CFModel*>("output_model");
  // By default, the main program use NMFPolicy.
  const mat w1 = outputModel->template CFPtr<NMFPolicy>()->Decomposition().W();
  const mat h1 = outputModel->template CFPtr<NMFPolicy>()->Decomposition().H();

  ResetSettings();

  // Do not set iteration_only_termination.
  SetInputParam("training", std::move(dataset));
  SetInputParam("max_iterations", int(100));
  SetInputParam("min_residue", double(1e9));

  // The execution of CF algorithm depends on initial random seed.
  mlpack::math::FixedRandomSeed();
  mlpackMain();

  outputModel = IO::GetParam<CFModel*>("output_model");
  // By default, the main program use NMFPolicy.
  const mat w2 = outputModel->template CFPtr<NMFPolicy>()->Decomposition().W();
  const mat h2 = outputModel->template CFPtr<NMFPolicy>()->Decomposition().H();

  // The resulting matrices should be different.
  BOOST_REQUIRE(arma::norm(w1 - w2) > 1e-5 || arma::norm(h1 - h2) > 1e-5);
}

/**
 * Test that max_iterations is used.
 */
BOOST_AUTO_TEST_CASE(CFMaxIterationsTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);
  const CFModel* outputModel;

  // Set a larger max_iterations.
  SetInputParam("max_iterations", int(100));
  SetInputParam("training", dataset);
  SetInputParam("iteration_only_termination", true);

  // The execution of CF algorithm depends on initial random seed.
  mlpack::math::FixedRandomSeed();
  mlpackMain();

  outputModel =  IO::GetParam<CFModel*>("output_model");
  // By default, the main program use NMFPolicy.
  const mat w1 = outputModel->template CFPtr<NMFPolicy>()->Decomposition().W();
  const mat h1 = outputModel->template CFPtr<NMFPolicy>()->Decomposition().H();

  ResetSettings();

  // Set a smaller max_iterations.
  SetInputParam("max_iterations", int(5));
  SetInputParam("training", std::move(dataset));
  SetInputParam("iteration_only_termination", true);

  // The execution of CF algorithm depends on initial random seed.
  mlpack::math::FixedRandomSeed();
  mlpackMain();

  outputModel = IO::GetParam<CFModel*>("output_model");
  // By default the main program use NMFPolicy.
  const mat w2 = outputModel->template CFPtr<NMFPolicy>()->Decomposition().W();
  const mat h2 = outputModel->template CFPtr<NMFPolicy>()->Decomposition().H();

  // The resulting matrices should be different.
  BOOST_REQUIRE(arma::norm(w1 - w2) > 1e-5 || arma::norm(h1 - h2) > 1e-5);
}

/**
 * Test that neighborhood is used.
 */
BOOST_AUTO_TEST_CASE(CFNeighborhoodTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);

  const int querySize = 7;
  Mat<size_t> query = arma::linspace<Mat<size_t>>(0, querySize - 1, querySize);

  SetInputParam("neighborhood", int(1));
  SetInputParam("training", dataset);
  SetInputParam("max_iterations", int(10));
  SetInputParam("query", query);

  // The execution of CF algorithm depends on initial random seed.
  mlpack::math::FixedRandomSeed();
  mlpackMain();

  const arma::Mat<size_t> output1 = IO::GetParam<arma::Mat<size_t>>("output");

  ResetSettings();

  // Set a different value for neighborhood.
  SetInputParam("neighborhood", int(10));
  SetInputParam("training", std::move(dataset));
  SetInputParam("max_iterations", int(10));
  SetInputParam("query", std::move(query));

  // The execution of CF algorithm depends on initial random seed.
  mlpack::math::FixedRandomSeed();
  mlpackMain();

  const arma::Mat<size_t> output2 = IO::GetParam<arma::Mat<size_t>>("output");

  // The resulting matrices should be different.
  BOOST_REQUIRE(arma::any(arma::vectorise(output1 != output2)));
}

/**
 * Ensure interpolation algorithm is one of { "average", "regression",
 * "similarity" }.
 */
BOOST_AUTO_TEST_CASE(CFInterpolationAlgorithmBoundTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);

  const int querySize = 7;
  Mat<size_t> query = arma::linspace<Mat<size_t>>(0, querySize - 1, querySize);

  // interpolation algorithm should be valid.
  SetInputParam("interpolation", std::string("invalid_algorithm"));
  SetInputParam("training", std::move(dataset));
  SetInputParam("query", query);

  Log::Fatal.ignoreInput = true;
  BOOST_REQUIRE_THROW(mlpackMain(), std::runtime_error);
  Log::Fatal.ignoreInput = false;
}

/**
 * Ensure that using interpolation algorithm makes a difference.
 */
BOOST_AUTO_TEST_CASE(CFInterpolationTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);

  const int querySize = 7;
  Mat<size_t> query = arma::linspace<Mat<size_t>>(0, querySize - 1, querySize);

  // Query with different interpolation types.
  ResetSettings();

  // Using average interpolation algorithm.
  SetInputParam("training", dataset);
  SetInputParam("max_iterations", int(10));
  SetInputParam("query", query);
  SetInputParam("interpolation", std::string("average"));
  SetInputParam("recommendations", 5);

  mlpackMain();

  const arma::Mat<size_t> output1 = IO::GetParam<arma::Mat<size_t>>("output");

  BOOST_REQUIRE_EQUAL(output1.n_rows, 5);
  BOOST_REQUIRE_EQUAL(output1.n_cols, 7);

  // Reset passed parameters.
  IO::GetSingleton().Parameters()["training"].wasPassed = false;
  IO::GetSingleton().Parameters()["max_iterations"].wasPassed = false;
  IO::GetSingleton().Parameters()["algorithm"].wasPassed = false;

  // Using regression interpolation algorithm.
  SetInputParam("input_model",
      std::move(IO::GetParam<CFModel*>("output_model")));
  SetInputParam("query", query);
  SetInputParam("interpolation", std::string("regression"));
  SetInputParam("recommendations", 5);

  mlpackMain();

  const arma::Mat<size_t> output2 = IO::GetParam<arma::Mat<size_t>>("output");

  BOOST_REQUIRE_EQUAL(output2.n_rows, 5);
  BOOST_REQUIRE_EQUAL(output2.n_cols, 7);

  // Using similarity interpolation algorithm.
  SetInputParam("input_model",
      std::move(IO::GetParam<CFModel*>("output_model")));
  SetInputParam("query", query);
  SetInputParam("interpolation", std::string("similarity"));
  SetInputParam("recommendations", 5);

  mlpackMain();

  const arma::Mat<size_t> output3 = IO::GetParam<arma::Mat<size_t>>("output");

  BOOST_REQUIRE_EQUAL(output3.n_rows, 5);
  BOOST_REQUIRE_EQUAL(output3.n_cols, 7);

  // The resulting matrices should be different.
  BOOST_REQUIRE(arma::any(arma::vectorise(output1 != output2)));
  BOOST_REQUIRE(arma::any(arma::vectorise(output1 != output3)));
}

/**
 * Ensure neighbor search algorithm is one of { "cosine", "euclidean",
 * "pearson" }.
 */
BOOST_AUTO_TEST_CASE(CFNeighborSearchAlgorithmBoundTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);

  const int querySize = 7;
  Mat<size_t> query = arma::linspace<Mat<size_t>>(0, querySize - 1, querySize);

  // neighbor search algorithm should be valid.
  SetInputParam("neighbor_search", std::string("invalid_algorithm"));
  SetInputParam("training", std::move(dataset));
  SetInputParam("query", query);

  Log::Fatal.ignoreInput = true;
  BOOST_REQUIRE_THROW(mlpackMain(), std::runtime_error);
  Log::Fatal.ignoreInput = false;
}

/**
 * Ensure that using neighbor search algorithm makes a difference.
 */
BOOST_AUTO_TEST_CASE(CFNeighborSearchTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);

  const int querySize = 7;
  Mat<size_t> query = arma::linspace<Mat<size_t>>(0, querySize - 1, querySize);

  // Query with different neighbor search types.
  ResetSettings();

  // Using euclidean neighbor search algorithm.
  SetInputParam("training", dataset);
  SetInputParam("max_iterations", int(10));
  SetInputParam("query", query);
  SetInputParam("neighbor_search", std::string("euclidean"));
  SetInputParam("recommendations", 5);

  mlpackMain();

  const arma::Mat<size_t> output1 = IO::GetParam<arma::Mat<size_t>>("output");

  BOOST_REQUIRE_EQUAL(output1.n_rows, 5);
  BOOST_REQUIRE_EQUAL(output1.n_cols, 7);

  // Reset passed parameters.
  IO::GetSingleton().Parameters()["training"].wasPassed = false;
  IO::GetSingleton().Parameters()["max_iterations"].wasPassed = false;
  IO::GetSingleton().Parameters()["algorithm"].wasPassed = false;

  // Using cosine neighbor search algorithm.
  SetInputParam("input_model",
      std::move(IO::GetParam<CFModel*>("output_model")));
  SetInputParam("query", query);
  SetInputParam("neighbor_search", std::string("cosine"));
  SetInputParam("recommendations", 5);

  mlpackMain();

  const arma::Mat<size_t> output2 = IO::GetParam<arma::Mat<size_t>>("output");

  BOOST_REQUIRE_EQUAL(output2.n_rows, 5);
  BOOST_REQUIRE_EQUAL(output2.n_cols, 7);

  // Using pearson neighbor search algorithm.
  SetInputParam("input_model",
      std::move(IO::GetParam<CFModel*>("output_model")));
  SetInputParam("query", query);
  SetInputParam("neighbor_search", std::string("pearson"));
  SetInputParam("recommendations", 5);

  mlpackMain();

  const arma::Mat<size_t> output3 = IO::GetParam<arma::Mat<size_t>>("output");

  BOOST_REQUIRE_EQUAL(output3.n_rows, 5);
  BOOST_REQUIRE_EQUAL(output3.n_cols, 7);

  // The resulting matrices should be different.
  BOOST_REQUIRE(arma::any(arma::vectorise(output1 != output2)));
  BOOST_REQUIRE(arma::any(arma::vectorise(output1 != output3)));
}

/**
 * Ensure normalization algorithm is one of { "none", "z_score",
 * "item_mean", "user_mean" }.
 */
BOOST_AUTO_TEST_CASE(CFNormalizationBoundTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);

  const int querySize = 7;
  Mat<size_t> query = arma::linspace<Mat<size_t>>(0, querySize - 1, querySize);

  SetInputParam("neighbor_search", std::string("cosine"));
  SetInputParam("algorithm", std::string("NMF"));

  // Normalization algorithm should be valid.
  SetInputParam("normalization", std::string("invalid_normalization"));
  SetInputParam("training", std::move(dataset));
  SetInputParam("query", query);

  Log::Fatal.ignoreInput = true;
  BOOST_REQUIRE_THROW(mlpackMain(), std::runtime_error);
  Log::Fatal.ignoreInput = false;
}

/**
 * Ensure that using normalization techniques make difference.
 */
BOOST_AUTO_TEST_CASE(CFNormalizationTest)
{
  mat dataset;
  data::Load("GroupLensSmall.csv", dataset);

  const int querySize = 7;
  Mat<size_t> query = arma::linspace<Mat<size_t>>(0, querySize - 1, querySize);

  // Query with different normalization techniques.
  ResetSettings();

  SetInputParam("training", dataset);
  SetInputParam("max_iterations", int(10));
  SetInputParam("query", query);
  SetInputParam("algorithm", std::string("NMF"));

  // Using without Normalization.
  SetInputParam("normalization", std::string("none"));
  SetInputParam("recommendations", 5);

  mlpackMain();

  const arma::Mat<size_t> output1 = IO::GetParam<arma::Mat<size_t>>("output");

  BOOST_REQUIRE_EQUAL(output1.n_rows, 5);
  BOOST_REQUIRE_EQUAL(output1.n_cols, 7);

  // Query with different normalization techniques.
  ResetSettings();

  SetInputParam("training", dataset);
  SetInputParam("max_iterations", int(10));
  SetInputParam("query", query);
  SetInputParam("algorithm", std::string("NMF"));

  // Using Item Mean normalization.
  SetInputParam("normalization", std::string("item_mean"));
  SetInputParam("recommendations", 5);

  mlpackMain();

  const arma::Mat<size_t> output2 = IO::GetParam<arma::Mat<size_t>>("output");

  BOOST_REQUIRE_EQUAL(output2.n_rows, 5);
  BOOST_REQUIRE_EQUAL(output2.n_cols, 7);

  // Query with different normalization techniques.
  ResetSettings();

  SetInputParam("training", dataset);
  SetInputParam("max_iterations", int(10));
  SetInputParam("query", query);
  SetInputParam("algorithm", std::string("NMF"));

  // Using Z-Score normalization.
  SetInputParam("normalization", std::string("z_score"));
  SetInputParam("recommendations", 5);

  mlpackMain();

  const arma::Mat<size_t> output3 = IO::GetParam<arma::Mat<size_t>>("output");

  BOOST_REQUIRE_EQUAL(output3.n_rows, 5);
  BOOST_REQUIRE_EQUAL(output3.n_cols, 7);

  // The resulting matrices should be different.
  BOOST_REQUIRE(arma::any(arma::vectorise(output1 != output2)));
  BOOST_REQUIRE(arma::any(arma::vectorise(output1 != output3)));
}

BOOST_AUTO_TEST_SUITE_END();
