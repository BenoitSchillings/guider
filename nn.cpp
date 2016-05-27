#include <iostream>
#include <sstream>
#include <time.h>
#include <stdexcept>

// OpenNN includes

#include "./opennn/opennn.h"

using namespace OpenNN;

int main(void)
{
   try
   {
      srand( (unsigned)time( NULL ) );
      DataSet data_set;

      data_set.set_data_file_name("./data.dat");

      data_set.load_data();

      Variables* variables_pointer = data_set.get_variables_pointer();

      variables_pointer->set_use(0, Variables::Input);
      variables_pointer->set_use(5, Variables::Target);


      Matrix<std::string> inputs_information = variables_pointer->arrange_inputs_information();
      Matrix<std::string> targets_information = variables_pointer->arrange_targets_information();

      Instances* instances_pointer = data_set.get_instances_pointer();

      instances_pointer->set_training();

      Vector< Statistics<double> > inputs_statistics = data_set.scale_inputs_minimum_maximum();
      Vector< Statistics<double> > targets_statistics = data_set.scale_targets_minimum_maximum();

      Vector<size_t> architecture(4);
      int i = 0;

      architecture[i++] = 5;
      architecture[i++] = 4;
      architecture[i++] = 4;
      architecture[i++] = 5;

      NeuralNetwork neural_network;
      neural_network.set(architecture);


      Inputs* inputs_pointer = neural_network.get_inputs_pointer();
      inputs_pointer->set_information(inputs_information);

      Outputs* outputs_pointer = neural_network.get_outputs_pointer();
      outputs_pointer->set_information(targets_information);

      neural_network.construct_scaling_layer();
      ScalingLayer* scaling_layer_pointer = neural_network.get_scaling_layer_pointer();
      scaling_layer_pointer->set_statistics(inputs_statistics);
      scaling_layer_pointer->set_scaling_method(ScalingLayer::NoScaling);

      neural_network.construct_unscaling_layer();
      UnscalingLayer* unscaling_layer_pointer = neural_network.get_unscaling_layer_pointer();
      unscaling_layer_pointer->set_statistics(targets_statistics);
      unscaling_layer_pointer->set_unscaling_method(UnscalingLayer::NoUnscaling);

      // Performance functional object
 
      PerformanceFunctional performance_functional(&neural_network, &data_set);

      // Training strategy

      TrainingStrategy training_strategy(&performance_functional);

      QuasiNewtonMethod* quasi_Newton_method_pointer = training_strategy.get_quasi_Newton_method_pointer();

      quasi_Newton_method_pointer->set_minimum_performance_increase(1.0e-6);

      TrainingStrategy::Results training_strategy_results = training_strategy.perform_training();

      // Testing analysis object

      instances_pointer->set_testing();

      TestingAnalysis testing_analysis(&neural_network, &data_set);

      TestingAnalysis::LinearRegressionResults linear_regression_results = testing_analysis.perform_linear_regression_analysis();
      
      // Save results

      scaling_layer_pointer->set_scaling_method(ScalingLayer::MinimumMaximum);
      unscaling_layer_pointer->set_unscaling_method(UnscalingLayer::MinimumMaximum);

      data_set.save("./data/data_set.xml");

      neural_network.save("./data/neural_network.xml");
      neural_network.save_expression("./data/expression.txt");

      Vector<double> inputs(2);
      inputs[0] = 0.428898;
      inputs[1] = 0.08097; 
      
      for (int i =0; i < 200; i++) {
      	Vector<double> outputs = neural_network.calculate_outputs(inputs);
      	printf("%f %f\n", outputs[0], outputs[1]); 
     	inputs[1] += 0.003; 
        //inputs[0] += 0.002; 
      }
 
      return(0);
   }
   catch(std::exception& e)
   {
      std::cerr << e.what() << std::endl;

      return(1);
   }
}  
