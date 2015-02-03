// Copyright Hugh Perkins 2014 hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License, 
// v. 2.0. If a copy of the MPL was not distributed with this file, You can 
// obtain one at http://mozilla.org/MPL/2.0/.

#include <stdexcept>
#include <random>

#include "Timer.h"
#include "ConvolutionalLayer.h"
#include "LayerMaker.h"
#include "ActivationFunction.h"
#include "StatefulTimer.h"
#include "AccuracyHelper.h"
#include "NeuralNetMould.h"
#include "Layer.h"
#include "InputLayer.h"
#include "FullyConnectedLayer.h"
#include "EpochMaker.h"
#include "LossLayer.h"
#include "IAcceptsLabels.h"
#include "ExceptionMacros.h"

#include "NeuralNet.h"

using namespace std;

//static std::mt19937 random;

#undef VIRTUAL
#define VIRTUAL
#undef STATIC
#define STATIC

NeuralNet::NeuralNet() {
//    cout << "NeuralNet()" << endl;
    cl = OpenCLHelper::createForFirstGpuOtherwiseCpu();
//    InputLayerMaker<T> *maker = new InputLayerMaker<T>( this, numPlanes, boardSize );
//    maker->insert();
}
NeuralNet::NeuralNet( int numPlanes, int boardSize ) {
//    cout << "NeuralNet(planes,boardsize)" << endl;
    cl = OpenCLHelper::createForFirstGpuOtherwiseCpu();
    InputLayerMaker<float> *maker = ( new InputLayerMaker<float>( this ) )
        ->numPlanes( numPlanes )->boardSize( boardSize );
    maker->insert();
}
NeuralNet::~NeuralNet() {
    for( int i = 0; i < layers.size(); i++ ) {
        delete layers[i];
    }
    delete cl;
}
NeuralNet *NeuralNet::clone() {
    NeuralNet *copy = new NeuralNet();
    Layer *previousLayer = 0;
    for( vector<Layer *>::iterator it = layers.begin(); it != layers.end(); it++ ) {
        LayerMaker const*maker = (*it)->maker;
        LayerMaker const*makerCopy = maker->clone( copy, previousLayer );
        Layer *layerCopy = makerCopy->instance();
        copy->layers.push_back( layerCopy );
        previousLayer = layerCopy;
    }
}
OpenCLHelper *NeuralNet::getCl() {
    return cl;
}
STATIC NeuralNetMould *NeuralNet::maker() {
    return new NeuralNetMould();
}
template< typename T >InputLayerMaker<T> *NeuralNet::inputMaker() {
    if( layers.size() != 0 ) {
        throw runtime_error("Already added an InputLayer to this net");
    }
    return new InputLayerMaker<T>( this );
}
FullyConnectedMaker *NeuralNet::fullyConnectedMaker() {
    return new FullyConnectedMaker( this, getLastLayer() );
}
ConvolutionalMaker *NeuralNet::convolutionalMaker() {
    return new ConvolutionalMaker( this, getLastLayer() );
}
PoolingMaker *NeuralNet::poolingMaker() {
    return new PoolingMaker( this, getLastLayer() );
}
NormalizationLayerMaker *NeuralNet::normalizationMaker() {
    return new NormalizationLayerMaker( this, getLastLayer() );
}
RandomPatchesMaker *NeuralNet::randomPatchesMaker() {
    return new RandomPatchesMaker( this, getLastLayer() );
}
RandomTranslationsMaker *NeuralNet::randomTranslationsMaker() {
    return new RandomTranslationsMaker( this, getLastLayer() );
}
SquareLossMaker *NeuralNet::squareLossMaker() {
    return new SquareLossMaker( this, getLastLayer() );
}
CrossEntropyLossMaker *NeuralNet::crossEntropyLossMaker() {
    return new CrossEntropyLossMaker( this, getLastLayer() );
}
SoftMaxMaker *NeuralNet::softMaxLossMaker() {
    return new SoftMaxMaker( this, getLastLayer() );
}
void NeuralNet::initWeights( int layerIndex, float *weights, float *biasWeights ) {
    initWeights( layerIndex, weights );
    initBiasWeights( layerIndex, biasWeights );
}
void NeuralNet::initWeights( int layerIndex, float *weights ) {
    layers[layerIndex]->initWeights( weights );
}
void NeuralNet::initBiasWeights( int layerIndex, float *weights ) {
    layers[layerIndex]->initBiasWeights( weights );
}
void NeuralNet::printWeightsAsCode() {
    for( int layer = 1; layer < layers.size(); layer++ ) {
        layers[layer]->printWeightsAsCode();
    }
}
void NeuralNet::printBiasWeightsAsCode() {
    for( int layer = 1; layer < layers.size(); layer++ ) {
        layers[layer]->printBiasWeightsAsCode();
    }
}
float NeuralNet::calcLoss(float const *expectedValues ) {
    return dynamic_cast<LossLayer*>(layers[layers.size()-1])->calcLoss( expectedValues );
}
float NeuralNet::calcLossFromLabels(int const *labels ) {
    return dynamic_cast<IAcceptsLabels*>(layers[layers.size()-1])->calcLossFromLabels( labels );
}
EpochMaker *NeuralNet::epochMaker() {
     return new EpochMaker(this);
}
template< typename T > InputLayer<T> *NeuralNet::getFirstLayer() {
    return dynamic_cast<InputLayer<T> *>( layers[0] );
}
Layer *NeuralNet::getLastLayer() {
    return layers[layers.size() - 1];
}
Layer *NeuralNet::addLayer( LayerMaker *maker ) {
    Layer *previousLayer = 0;
    if( layers.size() > 0 ) {
        previousLayer = layers[ layers.size() - 1 ];
    }
//    maker->setPreviousLayer( previousLayer );
    Layer *layer = maker->instance();
    layers.push_back( layer );
    return layer;
}
void NeuralNet::setBatchSize( int batchSize ) {
    for( std::vector<Layer*>::iterator it = layers.begin(); it != layers.end(); it++ ) {
        (*it)->setBatchSize( batchSize );
    }
}
void NeuralNet::setTraining( bool training ) {
    for( std::vector<Layer*>::iterator it = layers.begin(); it != layers.end(); it++ ) {
        (*it)->setTraining( training );
    }
}
int NeuralNet::calcNumRight( int const *labels ) {
    IAcceptsLabels *acceptsLabels = dynamic_cast<IAcceptsLabels*>(getLastLayer());
    if( acceptsLabels == 0 ) {
        THROW("You need to add a IAcceptsLabels as the last layer, in order to use calcNumRight");
    }
    return acceptsLabels->calcNumRight( labels );
}
template< typename T > void NeuralNet::propagate( T const*images) {
    // forward...
    dynamic_cast<InputLayer<T> *>(layers[0])->in( images );
    for( int layerId = 0; layerId < layers.size(); layerId++ ) {
        StatefulTimer::setPrefix("layer" + toString(layerId) + " " );
        layers[layerId]->propagate();
        StatefulTimer::setPrefix("" );
    }
}
void NeuralNet::backPropFromLabels( float learningRate, int const *labels) {
    IAcceptsLabels *acceptsLabels = dynamic_cast<IAcceptsLabels*>(getLastLayer());
    if( acceptsLabels == 0 ) {
        throw std::runtime_error("Must add a child of IAcceptsLabels as last layer, to use backPropFromLabels");
    }
    acceptsLabels->calcErrorsFromLabels( labels );
    for( int layerIdx = (int)layers.size() - 2; layerIdx >= 1; layerIdx-- ) { // no point in propagating to input layer :-P
        StatefulTimer::setPrefix("layer" + toString(layerIdx) + " " );
        Layer *layer = layers[layerIdx];
        if( layer->needsBackProp() ) {
            layer->backProp( learningRate );
        }
        StatefulTimer::setPrefix("" );
    }
}
void NeuralNet::backProp( float learningRate, float const *expectedResults) {
    LossLayer *lossLayer = dynamic_cast<LossLayer*>(getLastLayer());
    if( lossLayer == 0 ) {
        throw std::runtime_error("Must add a LossLayer as last layer of net");
    }
    lossLayer->calcErrors( expectedResults );
    for( int layerIdx = (int)layers.size() - 2; layerIdx >= 1; layerIdx-- ) { // no point in propagating to input layer :-P
        StatefulTimer::setPrefix("layer" + toString(layerIdx) + " " );
        layers[layerIdx]->backProp( learningRate );
        StatefulTimer::setPrefix("" );
    }
}
template< typename T > void NeuralNet::learnBatch( float learningRate, T const*images, float const *expectedResults ) {
    setTraining( true );
    propagate( images);
    backProp( learningRate, expectedResults );
}
template< typename T > void NeuralNet::learnBatchFromLabels( float learningRate, T const*images, int const *labels ) {
    setTraining( true );
    propagate( images);
    backPropFromLabels( learningRate, labels );
}
int NeuralNet::getNumLayers() {
    return (int)layers.size();
}
float const *NeuralNet::getResults( int layer ) const {
    return layers[layer]->getResults();
}
int NeuralNet::getInputCubeSize() const {
    return layers[ 0 ]->getOutputCubeSize();
}
int NeuralNet::getOutputCubeSize() const {
    return layers[ layers.size() - 1 ]->getOutputCubeSize();
}
float const *NeuralNet::getResults() const {
    return getResults( (int)layers.size() - 1 );
}
void NeuralNet::print() {
    int i = 0; 
    for( std::vector< Layer* >::iterator it = layers.begin(); it != layers.end(); it++ ) {
        std::cout << "layer " << i << ":" << (*it)->asString() << endl;
        i++;
    }
}
void NeuralNet::printWeights() {
    int i = 0; 
    for( std::vector< Layer* >::iterator it = layers.begin(); it != layers.end(); it++ ) {
        std::cout << "layer " << i << ":" << std::endl;
        (*it)->printWeights();
        i++;
    }
}
void NeuralNet::printOutput() {
    int i = 0; 
    for( std::vector< Layer* >::iterator it = layers.begin(); it != layers.end(); it++ ) {
        std::cout << "layer " << i << ":" << std::endl;
        (*it)->printOutput();
        i++;
    }
}

template ClConvolve_EXPORT InputLayerMaker<unsigned char> *NeuralNet::inputMaker<unsigned char>();
template ClConvolve_EXPORT InputLayerMaker<float> *NeuralNet::inputMaker<float>();
template ClConvolve_EXPORT void NeuralNet::propagate(unsigned char const*images);
template ClConvolve_EXPORT void NeuralNet::propagate(float const*images);
template ClConvolve_EXPORT void NeuralNet::learnBatchFromLabels<unsigned char>(float learningRate, unsigned char const*images, int const *labels);
template ClConvolve_EXPORT void NeuralNet::learnBatchFromLabels<float>(float learningRate, float const *images, int const *labels);
template ClConvolve_EXPORT void NeuralNet::learnBatch<unsigned char>(float learningRate, unsigned char const*images, float const *expectedResults );
template ClConvolve_EXPORT void NeuralNet::learnBatch<float>(float learningRate, float const *images, float const *expectedResults );


