#include <stdio.h>
#include "FANN/src/include/fann.h"
#include "FANNvars.h"
#include "vars.h"

int FANN_API test_callback(struct fann *ann, struct fann_train_data *train,
                           unsigned int max_epochs, unsigned int epochs_between_reports,
                           float desired_error, unsigned int epochs)
{
    printf("Epochs     %8d. MSE: %.5f. Desired-MSE: %.5f\n", epochs, fann_get_MSE(ann), desired_error);
    return 0;
}

int main()
{
    const float desired_error = (const float) 0.001;
    const unsigned int max_epochs = 500000;
    const unsigned int epochs_between_reports = 1000;
    fann_type *calc_out;
    struct fann *ann ;
    struct fann_train_data *data;
    unsigned int i = 0;

    printf("Creating network.\n");
    ann = fann_create_standard(LAYERS, 4+((NUMINPUTS-1) * DBINPUTS), HIDDEN, 2);

    data = fann_read_train_from_file("robottrain.data");

    fann_set_activation_steepness_hidden(ann, 1);
    fann_set_activation_steepness_output(ann, 1);

    fann_set_activation_function_hidden(ann, FANN_SIGMOID);
    fann_set_activation_function_output(ann, FANN_SIGMOID);
    fann_set_train_stop_function(ann, FANN_STOPFUNC_BIT);
    fann_set_bit_fail_limit(ann, 0.01f);

    fann_set_training_algorithm(ann, FANN_TRAIN_RPROP);

    fann_init_weights(ann, data);
    printf("Training network.\n");
    fann_train_on_data(ann, data, max_epochs, epochs_between_reports, desired_error);

    printf("Testing network. %f\n", fann_test_data(ann, data));

    for(i = 0; i < fann_length_train_data(data); i++)
    {
        calc_out = fann_run(ann, data->input[i]);
        printf("XOR test (%f,%f) -> %f, should be %f, difference=%f\n",
               data->input[i][0], data->input[i][1], calc_out[0], data->output[i][0],
               fann_abs(calc_out[0] - data->output[i][0]));
    }

    printf("Saving network.\n");
    fann_save(ann, "nueral.net");

    printf("Cleaning up.\n");
    fann_destroy_train(data);
    fann_destroy(ann);

    return 0;
}
