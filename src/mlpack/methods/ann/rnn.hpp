/**
 * @file rnn.hpp
 * @author Marcus Edel
 *
 * Definition of the RNN class, which implements recurrent neural networks.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#ifndef MLPACK_METHODS_ANN_RNN_HPP
#define MLPACK_METHODS_ANN_RNN_HPP

#include <mlpack/prereqs.hpp>

#include "visitor/delete_visitor.hpp"
#include "visitor/delta_visitor.hpp"
#include "visitor/output_parameter_visitor.hpp"
#include "visitor/reset_visitor.hpp"

#include "init_rules/network_init.hpp"

#include <mlpack/methods/ann/layer/layer_types.hpp>
#include <mlpack/methods/ann/layer/layer.hpp>
#include <mlpack/methods/ann/init_rules/random_init.hpp>

#include <ensmallen.hpp>

namespace mlpack {
namespace ann /** Artificial Neural Network. */ {

/**
 * Implementation of a standard recurrent neural network container.
 *
 * @tparam OutputLayerType The output layer type used to evaluate the network.
 * @tparam InitializationRuleType Rule used to initialize the weight matrix.
 */
template<
  typename OutputLayerType = NegativeLogLikelihood<>,
  typename InitializationRuleType = RandomInitialization,
  typename... CustomLayers
>
class RNN
{
 public:
  //! Convenience typedef for the internal model construction.
  using NetworkType = RNN<OutputLayerType,
                          InitializationRuleType,
                          CustomLayers...>;

  /**
   * Create the RNN object.
   *
   * Optionally, specify which initialize rule and performance function should
   * be used.
   *
   * If you want to pass in a parameter and discard the original parameter
   * object, be sure to use std::move to avoid unnecessary copy.
   *
   * @param rho Maximum number of steps to backpropagate through time (BPTT).
   * @param single Predict only the last element of the input sequence.
   * @param outputLayer Output layer used to evaluate the network.
   * @param initializeRule Optional instantiated InitializationRule object
   *        for initializing the network parameter.
   */
  RNN(const size_t rho,
      const bool single = false,
      OutputLayerType outputLayer = OutputLayerType(),
      InitializationRuleType initializeRule = InitializationRuleType());

  //! Destructor to release allocated memory.
  ~RNN();

  /**
   * Train the recurrent neural network on the given input data using the given
   * optimizer.
   *
   * This will use the existing model parameters as a starting point for the
   * optimization. If this is not what you want, then you should access the
   * parameters vector directly with Parameters() and modify it as desired.
   *
   * If you want to pass in a parameter and discard the original parameter
   * object, be sure to use std::move to avoid unnecessary copy.
   *
   * The format of the data should be as follows:
   *  - each slice should correspond to a time step
   *  - each column should correspond to a data point
   *  - each row should correspond to a dimension
   * So, e.g., predictors(i, j, k) is the i'th dimension of the j'th data point
   * at time slice k.
   *
   * @tparam OptimizerType Type of optimizer to use to train the model.
   * @tparam CallbackTypes Types of Callback Functions.
   * @param predictors Input training variables.
   * @param responses Outputs results from input training variables.
   * @param optimizer Instantiated optimizer used to train the model.
   * @param callbacks Callback functions.
   * @return The final objective of the trained model (NaN or Inf on error).
   */
  template<typename OptimizerType, typename... CallbackTypes>
  double Train(arma::cube predictors,
               arma::cube responses,
               OptimizerType& optimizer,
               CallbackTypes&&... callbacks);

  /**
   * Train the recurrent neural network on the given input data. By default, the
   * SGD optimization algorithm is used, but others can be specified
   * (such as ens::RMSprop).
   *
   * This will use the existing model parameters as a starting point for the
   * optimization. If this is not what you want, then you should access the
   * parameters vector directly with Parameters() and modify it as desired.
   *
   * If you want to pass in a parameter and discard the original parameter
   * object, be sure to use std::move to avoid unnecessary copy.
   *
   * The format of the data should be as follows:
   *  - each slice should correspond to a time step
   *  - each column should correspond to a data point
   *  - each row should correspond to a dimension
   * So, e.g., predictors(i, j, k) is the i'th dimension of the j'th data point
   * at time slice k.
   *
   * @tparam OptimizerType Type of optimizer to use to train the model.
   * @tparam CallbackTypes Types of Callback Functions.
   * @param predictors Input training variables.
   * @param responses Outputs results from input training variables.
   * @param callbacks Callback functions.
   * @return The final objective of the trained model (NaN or Inf on error).
   */
  template<typename OptimizerType = ens::StandardSGD, typename... CallbackTypes>
  double Train(arma::cube predictors,
               arma::cube responses,
               CallbackTypes&&... callbacks);

  /**
   * Predict the responses to a given set of predictors. The responses will
   * reflect the output of the given output layer as returned by the
   * output layer function.
   *
   * If you want to pass in a parameter and discard the original parameter
   * object, be sure to use std::move to avoid unnecessary copy.
   *
   * The format of the data should be as follows:
   *  - each slice should correspond to a time step
   *  - each column should correspond to a data point
   *  - each row should correspond to a dimension
   * So, e.g., predictors(i, j, k) is the i'th dimension of the j'th data point
   * at time slice k.  The responses will be in the same format.
   *
   * @param predictors Input predictors.
   * @param results Matrix to put output predictions of responses into.
   * @param batchSize Number of points to predict at once.
   */
  void Predict(arma::cube predictors,
               arma::cube& results,
               const size_t batchSize = 256);

  /**
   * Evaluate the recurrent neural network with the given parameters. This
   * function is usually called by the optimizer to train the model.
   *
   * @param parameters Matrix model parameters.
   * @param begin Index of the starting point to use for objective function
   *        evaluation.
   * @param batchSize Number of points to be passed at a time to use for
   *        objective function evaluation.
   * @param deterministic Whether or not to train or test the model. Note some
   *        layer act differently in training or testing mode.
   */
  double Evaluate(const arma::mat& parameters,
                  const size_t begin,
                  const size_t batchSize,
                  const bool deterministic);

  /**
   * Evaluate the recurrent neural network with the given parameters. This
   * function is usually called by the optimizer to train the model.  This just
   * calls the other overload of Evaluate() with deterministic = true.
   *
   * @param parameters Matrix model parameters.
   * @param begin Index of the starting point to use for objective function
   *        evaluation.
   * @param batchSize Number of points to be passed at a time to use for
   *        objective function evaluation.
   */
  double Evaluate(const arma::mat& parameters,
                  const size_t begin,
                  const size_t batchSize);

  /**
   * Evaluate the recurrent neural network with the given parameters. This
   * function is usually called by the optimizer to train the model.
   *
   * @param parameters Matrix model parameters.
   * @param begin Index of the starting point to use for objective function
   *        evaluation.
   * @param gradient Matrix to output gradient into.
   * @param batchSize Number of points to be passed at a time to use for
   *        objective function evaluation.
   */
  template<typename GradType>
  double EvaluateWithGradient(const arma::mat& parameters,
                              const size_t begin,
                              GradType& gradient,
                              const size_t batchSize);

  /**
   * Evaluate the gradient of the recurrent neural network with the given
   * parameters, and with respect to only one point in the dataset. This is
   * useful for optimizers such as SGD, which require a separable objective
   * function.
   *
   * @param parameters Matrix of the model parameters to be optimized.
   * @param begin Index of the starting point to use for objective function
   *        gradient evaluation.
   * @param gradient Matrix to output gradient into.
   * @param batchSize Number of points to be processed as a batch for objective
   *        function gradient evaluation.
   */
  void Gradient(const arma::mat& parameters,
                const size_t begin,
                arma::mat& gradient,
                const size_t batchSize);

  /**
   * Shuffle the order of function visitation. This may be called by the
   * optimizer.
   */
  void Shuffle();

  /*
   * Add a new module to the model.
   *
   * @param args The layer parameter.
   */
  template <class LayerType, class... Args>
  void Add(Args... args) { network.push_back(new LayerType(args...)); }

  /*
   * Add a new module to the model.
   *
   * @param layer The Layer to be added to the model.
   */
  void Add(LayerTypes<CustomLayers...> layer) { network.push_back(layer); }

  //! Return the number of separable functions (the number of predictor points).
  size_t NumFunctions() const { return numFunctions; }

  //! Return the initial point for the optimization.
  const arma::mat& Parameters() const { return parameter; }
  //! Modify the initial point for the optimization.
  arma::mat& Parameters() { return parameter; }

  //! Return the maximum length of backpropagation through time.
  const size_t& Rho() const { return rho; }
  //! Modify the maximum length of backpropagation through time.
  size_t& Rho() { return rho; }

  //! Get the matrix of responses to the input data points.
  const arma::cube& Responses() const { return responses; }
  //! Modify the matrix of responses to the input data points.
  arma::cube& Responses() { return responses; }

  //! Get the matrix of data points (predictors).
  const arma::cube& Predictors() const { return predictors; }
  //! Modify the matrix of data points (predictors).
  arma::cube& Predictors() { return predictors; }

  /**
   * Reset the state of the network.  This ensures that all internally-held
   * gradients are set to 0, all memory cells are reset, and the parameters
   * matrix is the right size.
   */
  void Reset();

  /**
   * Reset the module information (weights/parameters).
   */
  void ResetParameters();

  //! Serialize the model.
  template<typename Archive>
  void serialize(Archive& ar, const unsigned int /* version */);

 private:
  // Helper functions.
  /**
   * The Forward algorithm (part of the Forward-Backward algorithm).  Computes
   * forward probabilities for each module.
   *
   * @param input Data sequence to compute probabilities for.
   */
  void Forward(arma::mat&& input);

  /**
   * Reset the state of RNN cells in the network for new input sequence.
   */
  void ResetCells();

  /**
   * The Backward algorithm (part of the Forward-Backward algorithm). Computes
   * backward pass for module.
   */
  void Backward();

  /**
   * Iterate through all layer modules and update the the gradient using the
   * layer defined optimizer.
   */
  template<typename InputType>
  void Gradient(InputType&& input);

  /**
   * Reset the module status by setting the current deterministic parameter
   * for all modules that implement the Deterministic function.
   */
  void ResetDeterministic();

  /**
   * Reset the gradient for all modules that implement the Gradient function.
   */
  void ResetGradients(arma::mat& gradient);

  //! Number of steps to backpropagate through time (BPTT).
  size_t rho;

  //! Instantiated outputlayer used to evaluate the network.
  OutputLayerType outputLayer;

  //! Instantiated InitializationRule object for initializing the network
  //! parameter.
  InitializationRuleType initializeRule;

  //! The input size.
  size_t inputSize;

  //! The output size.
  size_t outputSize;

  //! The target size.
  size_t targetSize;

  //! Indicator if we already trained the model.
  bool reset;

    //! Only predict the last element of the input sequence.
  bool single;

  //! Locally-stored model modules.
  std::vector<LayerTypes<CustomLayers...> > network;

  //! The matrix of data points (predictors).
  arma::cube predictors;

  //! The matrix of responses to the input data points.
  arma::cube responses;

  //! Matrix of (trained) parameters.
  arma::mat parameter;

  //! The number of separable functions (the number of predictor points).
  size_t numFunctions;

  //! The current error for the backward pass.
  arma::mat error;

  //! Locally-stored delta visitor.
  DeltaVisitor deltaVisitor;

  //! Locally-stored output parameter visitor.
  OutputParameterVisitor outputParameterVisitor;

  //! List of all module parameters for the backward pass (BBTT).
  std::vector<arma::mat> moduleOutputParameter;

  //! Locally-stored weight size visitor.
  WeightSizeVisitor weightSizeVisitor;

  //! Locally-stored reset visitor.
  ResetVisitor resetVisitor;

  //! Locally-stored delete visitor.
  DeleteVisitor deleteVisitor;

  //! The current evaluation mode (training or testing).
  bool deterministic;

  //! The current gradient for the gradient pass.
  arma::mat currentGradient;

  // The BRN class should have access to internal members.
  template<
    typename OutputLayerType1,
    typename MergeLayerType1,
    typename MergeOutputType1,
    typename InitializationRuleType1,
    typename... CustomLayers1
  >
  friend class BRNN;
}; // class RNN

} // namespace ann
} // namespace mlpack

//! Set the serialization version of the RNN class.  Multiple template arguments
//! makes this ugly...
namespace boost {
namespace serialization {

template<typename OutputLayerType,
         typename InitializationRuleType,
         typename... CustomLayer>
struct version<
    mlpack::ann::RNN<OutputLayerType, InitializationRuleType, CustomLayer...>>
{
  BOOST_STATIC_CONSTANT(int, value = 1);
};

} // namespace serialization
} // namespace boost

// Include implementation.
#include "rnn_impl.hpp"

#endif
