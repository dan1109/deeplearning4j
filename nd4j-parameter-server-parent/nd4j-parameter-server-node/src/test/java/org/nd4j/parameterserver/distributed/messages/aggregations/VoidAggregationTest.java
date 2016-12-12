package org.nd4j.parameterserver.distributed.messages.aggregations;

import lombok.extern.slf4j.Slf4j;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.factory.Nd4j;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.*;

/**
 * @author raver119@gmail.com
 */
@Slf4j
public class VoidAggregationTest {
    private static final short NODES = 100;
    private static final int ELEMENTS_PER_NODE = 3;

    @Before
    public void setUp() throws Exception {

    }

    @After
    public void tearDown() throws Exception {

    }

    /**
     * In this test we check for aggregation of sample vector.
     *
     * @throws Exception
     */
    @Test
    public void getAccumulatedResult1() throws Exception {
        INDArray exp = Nd4j.linspace(0, (NODES * ELEMENTS_PER_NODE) - 1, NODES * ELEMENTS_PER_NODE);

        List<VectorAggregation> aggregations = new ArrayList<>();
        for (int i = 0, j = 0; i < NODES; i++) {

            INDArray array = Nd4j.create(ELEMENTS_PER_NODE);

            for (int e = 0; e < ELEMENTS_PER_NODE; j++, e++) {
                array.putScalar(e, (double) j);
            }
            VectorAggregation aggregation = new VectorAggregation(NODES, array);
            aggregation.setShardIndex((short) i);
            aggregations.add(aggregation);
        }


        VectorAggregation aggregation = aggregations.get(0);

        for (VectorAggregation vectorAggregation: aggregations) {
            aggregation.accumulateAggregation(vectorAggregation);
        }

        INDArray payload = aggregation.getAccumulatedResult();
        log.info("Payload shape: {}", payload.shape());
        assertEquals(exp, payload);
    }


    /**
     * This test checks for aggregation of single-array dot
     *
     * @throws Exception
     */
    @Test
    public void getScalarDotAggregation1() throws Exception {
        INDArray x = Nd4j.linspace(0, (NODES * ELEMENTS_PER_NODE) - 1, NODES * ELEMENTS_PER_NODE);
        INDArray y = x.dup();
        double exp = Nd4j.getBlasWrapper().dot(x, y);

        List<DotAggregation> aggregations = new ArrayList<>();
        for (int i = 0, j = 0; i < NODES; i++) {
            INDArray arrayX = Nd4j.create(ELEMENTS_PER_NODE);
            INDArray arrayY = Nd4j.create(ELEMENTS_PER_NODE);

            for (int e = 0; e < ELEMENTS_PER_NODE; j++, e++) {
                arrayX.putScalar(e, (double) j);
                arrayY.putScalar(e, (double) j);
            }

            double dot = Nd4j.getBlasWrapper().dot(arrayX, arrayY);

            DotAggregation aggregation = new DotAggregation(NODES, Nd4j.scalar(dot));
            aggregation.setShardIndex((short) i);
            aggregations.add(aggregation);
        }

        DotAggregation aggregation = aggregations.get(0);

        for (DotAggregation vectorAggregation: aggregations) {
            aggregation.accumulateAggregation(vectorAggregation);
        }

        INDArray result = aggregation.getAccumulatedResult();
        assertEquals(true, result.isScalar());
        assertEquals(exp, result.getDouble(0), 1e-5);
    }


    @Test
    public void getBatchedDotAggregation1() throws Exception {
        INDArray x = Nd4j.create(5, 300).assign(2.0);
        INDArray y = x.dup();

        x.muli(y);
        INDArray exp = x.sum(1);

        List<DotAggregation> aggregations = new ArrayList<>();
        for (int i = 0, j = 0; i < NODES; i++) {
            INDArray arrayX = Nd4j.create(5, ELEMENTS_PER_NODE);
            INDArray arrayY = Nd4j.create(5, ELEMENTS_PER_NODE);

            arrayX.assign(2.0);
            arrayY.assign(2.0);

            DotAggregation aggregation = new DotAggregation(NODES, arrayX.mul(arrayY));
            aggregation.setShardIndex((short) i);
            aggregations.add(aggregation);
        }

        DotAggregation aggregation = aggregations.get(0);

        for (DotAggregation vectorAggregation: aggregations) {
            aggregation.accumulateAggregation(vectorAggregation);
        }

        INDArray result = aggregation.getAccumulatedResult();
        assertArrayEquals(exp.shapeInfoDataBuffer().asInt(), result.shapeInfoDataBuffer().asInt());
        assertEquals(exp, result);
    }

}