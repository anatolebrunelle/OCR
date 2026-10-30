#include <math.h>
#include <stdio.h>

double sigmoid(double value)
{
    return 1.0 / (1.0 + exp(-value));
}

int main(void)
{
    double inputs[4][2] = {
        {0, 0}, {0, 1}, {1, 0}, {1, 1}
    };
    double expected[4] = {0, 1, 1, 0};

    double hidden_weights[2][2] = {
        {0.5, -0.4}, {-0.3, 0.6}
    };
    double hidden_bias[2] = {0.1, -0.2};
    double output_weights[2] = {0.7, -0.5};
    double output_bias = 0.1;
    double learning_rate = 0.8;

    for (int epoch = 0; epoch < 30000; epoch++)
    {
        for (int i = 0; i < 4; i++)
        {
            double hidden[2];
            for (int j = 0; j < 2; j++)
            {
                double sum = hidden_bias[j];
                for (int k = 0; k < 2; k++)
                    sum += inputs[i][k] * hidden_weights[j][k];
                hidden[j] = sigmoid(sum);
            }

            double sum = output_bias;
            for (int j = 0; j < 2; j++)
                sum += hidden[j] * output_weights[j];
            double output = sigmoid(sum);

            double output_delta = (expected[i] - output)
                                  * output * (1.0 - output);
            double hidden_delta[2];
            for (int j = 0; j < 2; j++)
                hidden_delta[j] = output_delta * output_weights[j]
                                  * hidden[j] * (1.0 - hidden[j]);

            for (int j = 0; j < 2; j++)
                output_weights[j] += learning_rate * output_delta * hidden[j];
            output_bias += learning_rate * output_delta;

            for (int j = 0; j < 2; j++)
            {
                for (int k = 0; k < 2; k++)
                    hidden_weights[j][k] += learning_rate * hidden_delta[j]
                                            * inputs[i][k];
                hidden_bias[j] += learning_rate * hidden_delta[j];
            }
        }
    }

    for (int i = 0; i < 4; i++)
    {
        double hidden[2];
        for (int j = 0; j < 2; j++)
        {
            double sum = hidden_bias[j];
            for (int k = 0; k < 2; k++)
                sum += inputs[i][k] * hidden_weights[j][k];
            hidden[j] = sigmoid(sum);
        }

        double sum = output_bias;
        for (int j = 0; j < 2; j++)
            sum += hidden[j] * output_weights[j];

        printf("%.0f XOR %.0f = %.4f (expected %.0f)\n",
               inputs[i][0], inputs[i][1], sigmoid(sum), expected[i]);
    }

    return 0;
}
