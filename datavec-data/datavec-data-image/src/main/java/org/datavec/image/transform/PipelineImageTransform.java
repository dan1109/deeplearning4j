/*-
 *  * Copyright 2016 Skymind, Inc.
 *  *
 *  *    Licensed under the Apache License, Version 2.0 (the "License");
 *  *    you may not use this file except in compliance with the License.
 *  *    You may obtain a copy of the License at
 *  *
 *  *        http://www.apache.org/licenses/LICENSE-2.0
 *  *
 *  *    Unless required by applicable law or agreed to in writing, software
 *  *    distributed under the License is distributed on an "AS IS" BASIS,
 *  *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  *    See the License for the specific language governing permissions and
 *  *    limitations under the License.
 */
package org.datavec.image.transform;

import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;

import org.datavec.api.berkeley.Pair;
import org.datavec.image.data.ImageWritable;
import org.nd4j.linalg.factory.Nd4j;

import static org.bytedeco.javacpp.opencv_core.*;

/**
 * Allows creation of image transform pipelines, either sequentially or randomly.
 *
 * You have the option of passing in multiple transforms as parameters. If you
 * want to create a more complex pipeline, you can pass in a pipeline that looks
 * like List<Pair<ImageTransform, Double>>. The Double is the probability that
 * particular element in the pipeline will be executed. This is helpful for creating
 * complex pipelines.
 *
 * The pipeline can also be randomly shuffled with each transform, further increasing
 * the available dataset.
 *
 * @author saudet
 * @author crockpotveggies
 */
public class PipelineImageTransform extends BaseImageTransform<Mat> {

    protected List<Pair<ImageTransform, Double>> imageTransforms;
    protected boolean shuffle;
    protected org.nd4j.linalg.api.rng.Random rng;


    public PipelineImageTransform(ImageTransform... transforms) {
        this(1234, false, transforms);
    }

    public PipelineImageTransform(long seed, boolean shuffle, ImageTransform... transforms) {
        super(null); // for perf reasons we ignore java Random, create our own

        List<Pair<ImageTransform, Double>> pipeline = new LinkedList<>();
        for(int i = 0; i < transforms.length; i++)
            pipeline.add(new Pair<>(transforms[i], 1.0));

        this.imageTransforms = pipeline;
        this.shuffle = shuffle;
        this.rng = Nd4j.getRandom();
        rng.setSeed(seed);
    }

    public PipelineImageTransform(List<Pair<ImageTransform, Double>> transforms) {
        this(1234, transforms, false);
    }

    public PipelineImageTransform(List<Pair<ImageTransform, Double>> transforms, boolean shuffle) {
        this(1234, transforms, shuffle);
    }

    public PipelineImageTransform(long seed, List<Pair<ImageTransform, Double>> transforms) {
        this(seed, transforms, false);
    }

    public PipelineImageTransform(long seed, List<Pair<ImageTransform, Double>> transforms, boolean shuffle) {
        this(null, seed, transforms, shuffle);
    }

    public PipelineImageTransform(Random random, long seed, List<Pair<ImageTransform, Double>> transforms, boolean shuffle) {
        super(null); // for perf reasons we ignore java Random, create our own
        this.imageTransforms = transforms;
        this.shuffle = shuffle;
        this.rng = Nd4j.getRandom();
        rng.setSeed(seed);
    }

    /**
     * Takes an image and executes a pipeline of combined transforms.
     *
     * @param image  to transform, null == end of stream
     * @param random object to use (or null for deterministic)
     * @return transformed image
     */
    @Override
    public ImageWritable transform(ImageWritable image, Random random) {
        if(shuffle) Collections.shuffle(imageTransforms);

        // execute each item in the pipeline
        for(Pair<ImageTransform, Double> tuple : imageTransforms) {
            if(tuple.getSecond() == 1.0 || rng.nextDouble() < tuple.getSecond()) { // probability of execution
                image = tuple.getFirst().transform(image);
            }
        }

        return image;
    }

}
