//
// @author raver119, created on 29/10/17.
// @author Yurii Shyrma, changed on 20.03.2018
//

#include <op_boilerplate.h>
#if NOT_EXCLUDED(OP_sconv2d)

#include <ops/declarable/CustomOperations.h>
#include <ops/declarable/generic/helpers/convolutions.h>
#include <memory>

namespace nd4j {
namespace ops  {


CUSTOM_OP_IMPL(sconv2d, 2, 1, false, 0, 9) {

    NDArray<T> *input        = INPUT_VARIABLE(0);                                           // [bS, iH, iW, iC]  (NHWC) or [bS, iC, iH, iW]  (NCHW)
    NDArray<T> *weightsDepth = INPUT_VARIABLE(1);                                           // [kH, kW, iC, mC]  (NHWC) or [mC, iC, kH, kW]  (NCHW)
    NDArray<T> *weightsPoint = nullptr;                                                     // [1, 1, iC*mC, oC] (NHWC) or [oC, iC*mC, 1, 1] (NCHW)
    NDArray<T> *bias         = nullptr;                                                     // [oC], oC = iC*mC if weightsPoint=nullptr

    NDArray<T> *output       = OUTPUT_VARIABLE(0);                                          // [bS, oH, oW, oC]  (NHWC) or [bS, oC, oH, oW]  (NCHW)

    if(block.width() == 3) {
        if((INPUT_VARIABLE(2))->rankOf() == 4)
            weightsPoint = INPUT_VARIABLE(2);
        else
            bias = INPUT_VARIABLE(2);
    }
    else if(block.width() == 4) {
        weightsPoint = INPUT_VARIABLE(2);
        bias = INPUT_VARIABLE(3);
    }

    REQUIRE_TRUE(input->rankOf()   == 4, 0, " SCONV2D OP: rank of input array must be equal to 4, but got %i instead !", input->rankOf());
    REQUIRE_TRUE(weightsDepth->rankOf() == 4, 0, " SCONV2D OP: rank of weightsDepth array must be equal to 4, but got %i instead !", weightsDepth->rankOf());
    if(weightsPoint)
        REQUIRE_TRUE(weightsPoint->rankOf() == 4, 0, " SCONV2D OP: rank of weightsPoint array must be equal to 4, but got %i instead !", weightsPoint->rankOf());
    if(bias)
        REQUIRE_TRUE(bias->rankOf() == 1 || bias->rankOf() == 2, 0, " SCONV2D OP: rank of biases array must be equal to 1 or 2, but got %i instead !", bias->rankOf());;

    int kH = INT_ARG(0);                                                        // filter(kernel) height
    int kW = INT_ARG(1);                                                        // filter(kernel) width
    int sH = INT_ARG(2);                                                        // strides height
    int sW = INT_ARG(3);                                                        // strides width
    int pH = INT_ARG(4);                                                        // paddings height
    int pW = INT_ARG(5);                                                        // paddings width
    int dH = INT_ARG(6);                                                        // dilations height
    int dW = INT_ARG(7);                                                        // dilations width
    int isSameMode = INT_ARG(8);                                                // 0-VALID, 1-SAME
    int isNCHW     = block.getIArguments()->size() > 9 ? !INT_ARG(9) : 1;       // 1-NCHW,  0-NHWC

    int bS, iC, iH, iW, mC, oC, oH, oW;                     // batch size, input channels, input height/width, channels multiplier, output channels, output height/width
    int indIOioC, indIiH, indWmC, indWiC, indWkH, indOoH;   // corresponding indexes
    ConvolutionUtils<T>::getSizesAndIndexesConv2d(isNCHW, *input, *output, bS, iC, iH, iW, oC, oH, oW, indIOioC, indIiH, indWiC, indWmC, indWkH, indOoH);
    mC = weightsDepth->sizeAt(indWmC);                      // channels multiplier

    std::string expectedWeightsDShape = ShapeUtils<T>::shapeAsString(ShapeUtils<T>::composeShapeUsingDimsAndIdx({mC,iC,kH,kW,  indWmC,indWiC,indWkH,indWkH+1}));
    REQUIRE_TRUE(expectedWeightsDShape == ShapeUtils<T>::shapeAsString(weightsDepth), 0, " SCONV2D OP: wrong shape of weightsDepth array, expected is %s, but got %s instead !", expectedWeightsDShape.c_str(), ShapeUtils<T>::shapeAsString(weightsDepth).c_str());
    if(weightsPoint) {
        std::string expectedWeightsPShape = ShapeUtils<T>::shapeAsString(ShapeUtils<T>::composeShapeUsingDimsAndIdx({oC,iC*mC,1,1,  indWmC,indWiC,indWkH,indWkH+1}));
        REQUIRE_TRUE(expectedWeightsPShape == ShapeUtils<T>::shapeAsString(weightsPoint), 0, " SCONV2D OP: wrong shape of weightsPoint array, expected is %s, but got %s instead !", expectedWeightsPShape.c_str(), ShapeUtils<T>::shapeAsString(weightsPoint).c_str());
    }
    if (bias)
        REQUIRE_TRUE(oC == bias->lengthOf(), 0, " SCONV2D OP: length of bias array must be equal to outChannels, but got %i instead", bias->lengthOf());

    if (iC == 1) {
        nd4j_debug("SCONV2D OP: for input_channels = 1 this op is equivalent to standard conv2d\n","");
        ConvolutionUtils<T>::conv2d({input, weightsDepth, bias}, output, {kH,kW, sH,sW, pH,pW, dH,dW, isSameMode, isNCHW});
        return Status::OK();
    }

    ConvolutionUtils<T>::sconv2d({input, weightsDepth, weightsPoint, bias}, output, {kH,kW, sH,sW, pH,pW, dH,dW, isSameMode, isNCHW});

    return Status::OK();
}


DECLARE_SHAPE_FN(sconv2d) {

    auto inputShapeInfo    = inputShape->at(0);         // [bS, iH, iW, iC]  (NHWC) or [bS, iC, iH, iW]  (NCHW)
    auto weightsDShapeInfo = inputShape->at(1);         // [kH, kW, iC, mC]  (NHWC) or [mC, iC, kH, kW]  (NCHW)
    Nd4jLong* weightsPShapeInfo = nullptr;                   // [1, 1, iC*mC, oC] (NHWC) or [oC, iC*mC, 1, 1] (NCHW)
    Nd4jLong* biasShapeInfo     = nullptr;                   // [oC], oC = iC*mC if weightsPoint=nullptr

    if(block.width() == 3)
        if(inputShape->at(2)[0] == 4)
            weightsPShapeInfo = inputShape->at(2);
        else
            biasShapeInfo = inputShape->at(2);
    else if(block.width() == 4) {
        weightsPShapeInfo = inputShape->at(2);
        biasShapeInfo = inputShape->at(3);
    }

    const int rank = 4;
    REQUIRE_TRUE(inputShapeInfo[0]   == rank, 0, "SCONV2D OP: rank of input array must be equal to %i, but got %i instead !", rank, inputShapeInfo[0]);
    REQUIRE_TRUE(weightsDShapeInfo[0] == rank, 0, "SCONV2D OP: rank of weightsDepth array must be equal to %i, but got %i instead !", rank, weightsDShapeInfo[0]);
    if(weightsPShapeInfo)
        REQUIRE_TRUE(weightsPShapeInfo[0] == rank, 0, "SCONV2D OP: rank of weightsPoint array must be equal to %i, but got %i instead !", rank, weightsPShapeInfo[0]);
    if(biasShapeInfo)
        REQUIRE_TRUE(biasShapeInfo[0] <= 2, 0, "SCONV2D OP: rank of biases array must be <= 2, but got %i instead !", biasShapeInfo[0]);;

    int kH = INT_ARG(0);                                                        // filter(kernel) height
    int kW = INT_ARG(1);                                                        // filter(kernel) width
    int sH = INT_ARG(2);                                                        // strides height
    int sW = INT_ARG(3);                                                        // strides width
    int pH = INT_ARG(4);                                                        // paddings height
    int pW = INT_ARG(5);                                                        // paddings width
    int dH = INT_ARG(6);                                                        // dilations height
    int dW = INT_ARG(7);                                                        // dilations width
    int isSameMode = INT_ARG(8);                                                // 0-VALID, 1-SAME
    int isNCHW  = block.getIArguments()->size() > 9 ? !INT_ARG(9) : 1;          // 0-NDHWC, 1-NCDHW

    int indIOioC, indIiH, indWkH, indWmC, indWiC;
    if(!isNCHW) {
        indIOioC = 3; indIiH = 1; indWkH = 0; indWmC = 3; indWiC = 2;
    }
    else {
        indIOioC = 1; indIiH = 2; indWkH = 2; indWmC = 0; indWiC = 1;
    }

    const int bS = inputShapeInfo[1];                                               // batch size
    const int iH = inputShapeInfo[indIiH+1];                                        // input height
    const int iW = inputShapeInfo[indIiH+2];                                        // input width
    const int iC = inputShapeInfo[indIOioC+1];                                      // input channels
    const int mC = weightsDShapeInfo[indWmC+1];                                      // channel multiplier
    const int oC = weightsPShapeInfo ? weightsPShapeInfo[indWmC+1] : iC*mC;       // output channels (oC or iC*mC)

    std::string expectedWeightsDShape = ShapeUtils<T>::shapeAsString(ShapeUtils<T>::composeShapeUsingDimsAndIdx({iC,mC,kH,kW,  indWiC,indWmC,indWkH,indWkH+1}));
    REQUIRE_TRUE(expectedWeightsDShape == ShapeUtils<T>::shapeAsString(weightsDShapeInfo), 0, "SCONV2D OP: wrong shape of depth weights array, expected is %s, but got %s instead !", expectedWeightsDShape.c_str(), ShapeUtils<T>::shapeAsString(weightsDShapeInfo).c_str());
    if(weightsPShapeInfo) {
        std::string expectedWeightsPShape = ShapeUtils<T>::shapeAsString(ShapeUtils<T>::composeShapeUsingDimsAndIdx({iC*mC,oC,1,1,  indWiC,indWmC,indWkH,indWkH+1}));
        REQUIRE_TRUE(expectedWeightsPShape == ShapeUtils<T>::shapeAsString(weightsPShapeInfo), 0, "SCONV2D OP: wrong shape of point array, expected is %s, but got %s instead !", expectedWeightsPShape.c_str(), ShapeUtils<T>::shapeAsString(weightsPShapeInfo).c_str());
    }
    if (biasShapeInfo)
        REQUIRE_TRUE(biasShapeInfo[0] <= 2 && oC == shape::length(biasShapeInfo), 0, "SCONV2D OP: wrong shape of array with biases, expected rank, length: <=2, %i, but got %i, %i instead !", oC, biasShapeInfo[0], shape::length(biasShapeInfo));

    int oH, oW;                                         // output height, width
    ConvolutionUtils<T>::calcOutSizePool2D(oH, oW, kH, kW, sH, sW, pH, pW, dH, dW, iH, iW, isSameMode);

    Nd4jLong* outputShapeInfo = nullptr;
    ALLOCATE(outputShapeInfo, block.getWorkspace(), shape::shapeInfoLength(inputShapeInfo), Nd4jLong);

    outputShapeInfo[0] = 4;
    outputShapeInfo[1] = bS;

    if (isNCHW) {
        outputShapeInfo[2] = oC;
        outputShapeInfo[3] = oH;
        outputShapeInfo[4] = oW;
    } else {
        outputShapeInfo[2] = oH;
        outputShapeInfo[3] = oW;
        outputShapeInfo[4] = oC;
    }

    shape::updateStrides(outputShapeInfo, shape::order(inputShapeInfo));

    return SHAPELIST(outputShapeInfo);
}


////////////////////////////////////////////////////////////////////////
CUSTOM_OP_IMPL(sconv2d_bp, 3, 2, false, 0, 9) {

    NDArray<T> *input        = INPUT_VARIABLE(0);                                           // [bS, iH, iW, iC]  (NHWC) or [bS, iC, iH, iW]  (NCHW)
    NDArray<T> *gradO        = INPUT_VARIABLE(1);                                           // [bS, oH, oW, oC]  (NHWC) or [bS, oC, oH, oW] (NCHW), epsilon_next
    NDArray<T> *weightsDepth = INPUT_VARIABLE(2);                                           // [kH, kW, iC, mC]  (NHWC) or [mC, iC, kH, kW]  (NCHW)
    NDArray<T> *weightsPoint = nullptr;                                                     // [1, 1, iC*mC, oC] (NHWC) or [oC, iC*mC, 1, 1] (NCHW)
    NDArray<T> *bias         = nullptr;                                                     // [oC], oC = iC*mC if weightsPoint=nullptr

    NDArray<T> *gradI  = OUTPUT_VARIABLE(0);                                                // [bS, iH, iW, iC]  (NHWC) or [bS, iC, iH, iW] (NCHW), epsilon
    NDArray<T> *gradWD = OUTPUT_VARIABLE(1);                                                // [kH, kW, iC, mC]  (NHWC) or [mC, iC, kH, kW] (NCHW)
    NDArray<T> *gradWP = nullptr;                                                           // [1, 1, iC*mC, oC] (NHWC) or [oC, iC*mC, 1, 1] (NCHW)
    NDArray<T> *gradB  = nullptr;                                                           // [oC]

    if(block.width() == 4) {
        if((INPUT_VARIABLE(3))->rankOf() == 4) {
            weightsPoint = INPUT_VARIABLE(3);
            gradWP       = OUTPUT_VARIABLE(2);
        }
        else {
            bias  = INPUT_VARIABLE(3);
            gradB = OUTPUT_VARIABLE(2);
        }
    }
    else if(block.width() == 5) {
        weightsPoint = INPUT_VARIABLE(3);
        bias         = INPUT_VARIABLE(4);
        gradWP       = OUTPUT_VARIABLE(2);
        gradB        = OUTPUT_VARIABLE(3);
    }


    REQUIRE_TRUE(input->rankOf()   == 4, 0, " SCONV2D_BP OP: rank of input array must be equal to 4, but got %i instead !", input->rankOf());
    REQUIRE_TRUE(gradO->rankOf()   == 4, 0, " SCONV2D_BP OP: rank of output gradients (next epsilon) array must be equal to 4, but got %i instead !", gradO->rankOf());
    REQUIRE_TRUE(weightsDepth->rankOf() == 4, 0, " SCONV2D_BP OP: rank of weightsDepth array must be equal to 4 !, but got %i instead !", weightsDepth->rankOf());
    if(weightsPoint) {
        REQUIRE_TRUE(weightsPoint->rankOf() == 4, 0, " SCONV2D_BP OP: rank of weightsPoint array must be equal to 4, but got %i instead !", weightsPoint->rankOf());
        REQUIRE_TRUE(gradWP->rankOf() == 4, 0, " SCONV2D_BP OP: rank of weightsPoint gradients array must be equal to 4, but got %i instead !", gradWP->rankOf());
    }
    if(bias) {
        REQUIRE_TRUE(bias->rankOf() == 1  || bias->rankOf()  == 2, 0, " SCONV2D_BP OP: rank of biases array must be equal to 1 or 2, but got %i instead !", bias->rankOf());
        REQUIRE_TRUE(gradB->rankOf() == 1 || gradB->rankOf() == 2, 0, " SCONV2D_BP OP: rank of  biases gradientsarray must be equal to 1 or 2, but got %i instead !", gradB->rankOf());
    }

    int kH = INT_ARG(0);                                                        // filter(kernel) height
    int kW = INT_ARG(1);                                                        // filter(kernel) width
    int sH = INT_ARG(2);                                                        // strides height
    int sW = INT_ARG(3);                                                        // strides width
    int pH = INT_ARG(4);                                                        // paddings height
    int pW = INT_ARG(5);                                                        // paddings width
    int dH = INT_ARG(6);                                                        // dilations height
    int dW = INT_ARG(7);                                                        // dilations width
    int isSameMode = INT_ARG(8);                                                // 0-VALID, 1-SAME
    int isNCHW     = block.getIArguments()->size() > 9 ? !INT_ARG(9) : 1;       // 1-NCHW,  0-NHWC

    int bS, iC, iH, iW, mC, oC, oH, oW;                     // batch size, input channels, input height/width, channels multiplier, output channels, output height/width
    int indIOioC, indIiH, indWmC, indWiC, indWkH, indOoH;   // corresponding indexes
    ConvolutionUtils<T>::getSizesAndIndexesConv2d(isNCHW, *input, *gradO, bS, iC, iH, iW, oC, oH, oW, indIOioC, indIiH, indWiC, indWmC, indWkH, indOoH);
    mC = weightsDepth->sizeAt(indWmC);                      // channels multiplier

    std::string expectedWeightsDShape = ShapeUtils<T>::shapeAsString(ShapeUtils<T>::composeShapeUsingDimsAndIdx({mC,iC,kH,kW,  indWmC,indWiC,indWkH,indWkH+1}));
    REQUIRE_TRUE(expectedWeightsDShape == ShapeUtils<T>::shapeAsString(weightsDepth), 0, " SCONV2D_BP OP: wrong shape of weightsDepth array, expected is %s, but got %s instead !", expectedWeightsDShape.c_str(), ShapeUtils<T>::shapeAsString(weightsDepth).c_str());
    REQUIRE_TRUE(expectedWeightsDShape == ShapeUtils<T>::shapeAsString(gradWD),       0, " SCONV2D_BP OP: wrong shape of gradWD array, expected is %s, but got %s instead !", expectedWeightsDShape.c_str(), ShapeUtils<T>::shapeAsString(gradWD).c_str());
    if(weightsPoint) {
        std::string expectedWeightsPShape = ShapeUtils<T>::shapeAsString(ShapeUtils<T>::composeShapeUsingDimsAndIdx({oC,iC*mC,1,1,  indWmC,indWiC,indWkH,indWkH+1}));
        REQUIRE_TRUE(expectedWeightsPShape == ShapeUtils<T>::shapeAsString(weightsPoint), 0, " SCONV2D_BP OP: wrong shape of weightsPoint array, expected is %s, but got %s instead !", expectedWeightsPShape.c_str(), ShapeUtils<T>::shapeAsString(weightsPoint).c_str());
        REQUIRE_TRUE(expectedWeightsPShape == ShapeUtils<T>::shapeAsString(gradWP),       0, " SCONV2D_BP OP: wrong shape of gradWP array, expected is %s, but got %s instead !", expectedWeightsPShape.c_str(), ShapeUtils<T>::shapeAsString(gradWP).c_str());
    }
    if (bias) {
        REQUIRE_TRUE(oC == bias->lengthOf(),  0, " SCONV2D_BP OP: length of bias array must be equal to outChannels, but got %i instead", bias->lengthOf());
        REQUIRE_TRUE(oC == gradB->lengthOf(), 0, " SCONV2D_BP OP: length of biases gradients array must be equal to outChannels, but got %i instead", gradB->lengthOf());
    }

    // if (iC == 1) {
    //     nd4j_debug(" SCONV2D_BP OP: for input_channels=1 this op is equivalent to standard conv2d_bp \n","");
    //     nd4j::ops::conv2d_bp<T> op;
    //     return op.execute(&block);
    // }

    // ----- if weightsPoint is present, perform pointwise backprop first and calculate gradWP at this step ----- //
    if (weightsPoint){

        auto resultFFShape = isNCHW ? std::vector<Nd4jLong>({bS, mC*iC, oH, oW}) : std::vector<Nd4jLong>({bS, oH, oW, mC*iC});
        NDArray<T>* resultFF = new NDArray<T>(input->ordering(), resultFFShape, block.getWorkspace());
        ConvolutionUtils<T>::sconv2d({input, weightsDepth, nullptr, nullptr}, resultFF, {kH,kW, sH,sW, pH,pW, dH,dW, isSameMode, isNCHW});

        auto gradIDepthShape = ShapeUtils<T>::composeShapeUsingDimsAndIdx({bS,iC*mC,oH,oW,  0,indIOioC,indIiH,indIiH+1});
        NDArray<T>* gradIDepth = new NDArray<T>(resultFF->ordering(), gradIDepthShape, block.getWorkspace());                 // [bS, oH, oW, iC*mC]  (NHWC) or [bS, iC*mC, oH, oW] (NCHW)

        ConvolutionUtils<T>::conv2dBP({resultFF, weightsPoint, bias, gradO}, {gradIDepth, gradWP, gradB}, {1,1, 1,1, 0,0, 1,1, isSameMode, isNCHW});    // in this case oH=iH and oW=iW

        gradO = gradIDepth;
        bias = gradB = nullptr;                     // if pointwise backprop was done then don't calculate gradB at depthwise_conv2d_bp step

        delete resultFF;
    }

    // ----- apply depthwise_conv2d_bp ----- //
    ConvolutionUtils<T>::depthwiseConv2dBP({input, weightsDepth, bias, gradO}, {gradI, gradWD, gradB}, {kH,kW, sH,sW, pH,pW, dH,dW, isSameMode, isNCHW});

    if(weightsPoint)
        delete gradO;

    return Status::OK();
}


DECLARE_SHAPE_FN(sconv2d_bp) {

    auto inputShapeInfo    = inputShape->at(0);                 // [bS, iH, iW, iC]  (NHWC) or [bS, iC, iH, iW]  (NCHW)
    auto gradOShapeInfo    = inputShape->at(1);                 // [bS, oH, oW, oC]  (NHWC) or [bS, oC, oH, oW] (NCHW), epsilon_next
    auto weightsDShapeInfo = inputShape->at(2);                 // [kH, kW, iC, mC]  (NHWC) or [mC, iC, kH, kW]  (NCHW)
    Nd4jLong* weightsPShapeInfo = nullptr;                           // [1, 1, iC*mC, oC] (NHWC) or [oC, iC*mC, 1, 1] (NCHW)
    Nd4jLong* biasShapeInfo     = nullptr;                           // [oC], oC = iC*mC if weightsPoint=nullptr

    if(block.width() == 4) {
        if(inputShape->at(3)[0] == 4)
            weightsPShapeInfo = inputShape->at(3);
        else
            biasShapeInfo  = inputShape->at(3);
    }
    else if(block.width() == 5) {
        weightsPShapeInfo = inputShape->at(3);
        biasShapeInfo         = inputShape->at(4);
    }

    const int rank = 4;
    REQUIRE_TRUE(inputShapeInfo[0]    == rank, 0, " SCONV2D_BP OP: rank of input array must be equal to %i, but got %i instead !", rank, inputShapeInfo[0]);
    REQUIRE_TRUE(gradOShapeInfo[0]    == rank, 0, " SCONV2D_BP OP: rank of output gradients (next epsilon) array must be equal to %i, but got %i instead !", rank, gradOShapeInfo[0]);
    REQUIRE_TRUE(weightsDShapeInfo[0] == rank, 0, " SCONV2D_BP OP: rank of weightsDepth array must be equal to %i, but got %i instead !", rank, weightsDShapeInfo[0]);
    if(weightsPShapeInfo)
        REQUIRE_TRUE(weightsPShapeInfo[0] == rank, 0, " SCONV2D_BP OP: rank of weightsPoint array must be equal to %i, but got %i instead !", rank, weightsPShapeInfo[0]);
    if(biasShapeInfo)
        REQUIRE_TRUE(biasShapeInfo[0] ==1 || biasShapeInfo[0] == 2, 0, " SCONV2D_BP OP: rank of biases array must be 1 or 2, but got %i instead !", biasShapeInfo[0]);;

    int kH = INT_ARG(0);                                                        // filter(kernel) height
    int kW = INT_ARG(1);                                                        // filter(kernel) width
    int sH = INT_ARG(2);                                                        // strides height
    int sW = INT_ARG(3);                                                        // strides width
    int pH = INT_ARG(4);                                                        // paddings height
    int pW = INT_ARG(5);                                                        // paddings width
    int dH = INT_ARG(6);                                                        // dilations height
    int dW = INT_ARG(7);                                                        // dilations width
    int isSameMode = INT_ARG(8);                                                // 0-VALID, 1-SAME
    int isNCHW     = block.getIArguments()->size() > 9 ? !INT_ARG(9) : 1;       // 1-NCHW,  0-NHWC

    int indIOioC, indIiH, indWkH, indWmC, indWiC;
    if(!isNCHW) {
        indIOioC = 3; indIiH = 1; indWkH = 0; indWmC = 3; indWiC = 2;
    }
    else {
        indIOioC = 1; indIiH = 2; indWkH = 2; indWmC = 0; indWiC = 1;
    }

    const int bS = inputShapeInfo[1];                                               // batch size
    const int iH = inputShapeInfo[indIiH+1];                                        // input height
    const int iW = inputShapeInfo[indIiH+2];                                        // input width
    const int iC = inputShapeInfo[indIOioC+1];                                      // input channels
    const int mC = weightsDShapeInfo[indWmC+1];                                     // channel multiplier
    const int oC = weightsPShapeInfo ? weightsPShapeInfo[indWmC+1] : iC*mC;         // output channels (oC or iC*mC)

    int trueoH, trueoW;          // true output height, width
    ConvolutionUtils<T>::calcOutSizePool2D(trueoH, trueoW, kH, kW, sH, sW, pH, pW, dH, dW, iH, iW, isSameMode);

    std::string expectedGradOShapeInfo = ShapeUtils<T>::shapeAsString(ShapeUtils<T>::composeShapeUsingDimsAndIdx({bS,oC,trueoH,trueoW,  0,indIOioC,indIiH,indIiH+1}));
    REQUIRE_TRUE(expectedGradOShapeInfo == ShapeUtils<T>::shapeAsString(gradOShapeInfo), 0, "SCONV2D_BP OP: wrong shape of output gradients (next epsilon) array, expected is %s, but got %s instead !", expectedGradOShapeInfo.c_str(), ShapeUtils<T>::shapeAsString(gradOShapeInfo).c_str());
    std::string expectedWeightsDShape = ShapeUtils<T>::shapeAsString(ShapeUtils<T>::composeShapeUsingDimsAndIdx({iC,mC,kH,kW,  indWiC,indWmC,indWkH,indWkH+1}));
    REQUIRE_TRUE(expectedWeightsDShape == ShapeUtils<T>::shapeAsString(weightsDShapeInfo), 0, "SCONV2D_BP OP: wrong shape of depth weights array, expected is %s, but got %s instead !", expectedWeightsDShape.c_str(), ShapeUtils<T>::shapeAsString(weightsDShapeInfo).c_str());
    if(weightsPShapeInfo) {
        std::string expectedWeightsPShape = ShapeUtils<T>::shapeAsString(ShapeUtils<T>::composeShapeUsingDimsAndIdx({iC*mC,oC,1,1,  indWiC,indWmC,indWkH,indWkH+1}));
        REQUIRE_TRUE(expectedWeightsPShape == ShapeUtils<T>::shapeAsString(weightsPShapeInfo), 0, "SCONV2D_BP OP: wrong shape of point array, expected is %s, but got %s instead !", expectedWeightsPShape.c_str(), ShapeUtils<T>::shapeAsString(weightsPShapeInfo).c_str());
    }
    if (biasShapeInfo)
        REQUIRE_TRUE((biasShapeInfo[0] == 1 || biasShapeInfo[0] == 2) && oC == shape::length(biasShapeInfo), 0, "SCONV2D_BP OP: wrong shape of array with biases, expected rank, length: <=2, %i, but got %i, %i instead !", oC, biasShapeInfo[0], shape::length(biasShapeInfo));

    Nd4jLong* gradIshapeInfo(nullptr), *gradWDshapeInfo(nullptr);
    COPY_SHAPE(inputShapeInfo, gradIshapeInfo);
    COPY_SHAPE(weightsDShapeInfo, gradWDshapeInfo);

    Nd4jLong* gradWPshapeInfo(nullptr), *gradBshapeInfo(nullptr);

    if(weightsPShapeInfo && biasShapeInfo) {
        COPY_SHAPE(weightsPShapeInfo, gradWPshapeInfo);
        COPY_SHAPE(biasShapeInfo, gradBshapeInfo);
        return SHAPELIST(gradIshapeInfo, gradWDshapeInfo, gradWPshapeInfo, gradBshapeInfo);
    }

    if(weightsPShapeInfo && !biasShapeInfo) {
        COPY_SHAPE(weightsPShapeInfo, gradWPshapeInfo);
        return SHAPELIST(gradIshapeInfo, gradWDshapeInfo, gradWPshapeInfo);
    }

    if(!weightsPShapeInfo && biasShapeInfo) {
        COPY_SHAPE(biasShapeInfo, gradBshapeInfo);
        return SHAPELIST(gradIshapeInfo, gradWDshapeInfo, gradBshapeInfo);
    }

    return SHAPELIST(gradIshapeInfo, gradWDshapeInfo);
}



}
}

#endif