import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.concurrent.ThreadLocalRandom;
import dk.brics.automaton.Automaton;
import dk.brics.automaton.RegExp;
import dk.brics.automaton.State;
import dk.brics.automaton.Transition;

import org.json.JSONArray;
import org.json.JSONObject;

public class InputGenerator {
    public static int num_of_examples_per_cluster = 5;
    // Convert a regular expression to a DFA
    private static Automaton regexToDFA(String regex) {
        RegExp exp = new RegExp(regex);
        Automaton a = exp.toAutomaton();
        if(!a.isDeterministic()) {
			a.determinize();
			a.minimize();
		}
        a.reduce();
        return a;
    }

    static private String mutateChar(String example, int index, Transition t, ArrayList<Character> chars) {
        char min = t.getMin();
        if (min < ' ') {
            min = ' ';
        }
        char max = t.getMax();
        if (max > '~') {
            max = '~';
        }

        // add this range to chars
        for (char c = min; c <= max; c++) {
            chars.remove((Character) c);
        }

        int randomNum = ThreadLocalRandom.current().nextInt(min, max + 1);
        char c = (char) randomNum;
        String s;
        if (index == example.length() - 1) {
            s = example.substring(0, index) + c;
        } else {
            s = example.substring(0, index) + c + example.substring(index + 1);
        }
        return s;
    }
    // Generate a set of positive and negative examples from a given input example and regular expression
    public static  Map<String, Map<String, Boolean>> generateExamples(String example, String regex,Automaton dfa ) {
        Map<String, Map<String, Boolean>> clusters = new LinkedHashMap<String, Map<String, Boolean>>();
        State p = dfa.getInitialState();
        for (int i = 0; i < example.length(); i++) {
            State q = p.step(example.charAt(i));
            // mutate this i-th character to generate positive example
			HashSet<String> positives = new HashSet<String>();
			HashSet<String> negatives = new HashSet<String>();
			ArrayList<Character> not_accepted_chars = new ArrayList<Character>();
			// find out what char ranges are not accepted by this state
            for (char c = ' '; c <= '~'; c++) {
                not_accepted_chars.add(c);
            }
			for(Transition t : p.getTransitions()) {
				if(t.getDest().equals(q)) {
					for(int j = 0; j < num_of_examples_per_cluster; j++) {
						String pos = mutateChar(example, i, t, not_accepted_chars);
						positives.add(pos);
					}
				} else {
					// another branch, may or may not be accepted after mutation
					for(int j = 0; j < num_of_examples_per_cluster; j++) {
						String s = mutateChar(example, i, t, not_accepted_chars);
						if(dfa.run(s)) {
							positives.add(s);
						} else {
							negatives.add(s);
						}
					}
				}
			}
			// here we mutate to generate negative examples
			if(!not_accepted_chars.isEmpty()) {
				for(int j = 0; j < num_of_examples_per_cluster; j++) {
					int random_num = ThreadLocalRandom.current().nextInt(0, not_accepted_chars.size());
					char mut = not_accepted_chars.get(random_num);
					String s;
					if(i == example.length() - 1) {
						s = example.substring(0, i) + mut;
					} else {
						s = example.substring(0, i) + mut + example.substring(i+1);
					}
					if(!dfa.run(s)){
						negatives.add(s);
					}
				}
			}
			Map<String, Boolean> cluster = new HashMap<String, Boolean>();

        	for(String positive : positives) {
        		cluster.put(positive , true);

        	}
        	if(negatives.isEmpty() && !positives.isEmpty()) {
        		// we didn't find any negative examples
        		String s = "Positive examples only";
        		clusters.put(s, cluster);
        	} else {
        		for(String negative: negatives) {

        			// If this is a negative example
					// get the index of the failure-inducing character
					// and append it to the end
					int index = getIndexOfFailure(dfa, negative);

            		cluster.put(negative ,false);

            		String explanation =
            				ExplanationGenerator.generateExplanation(negative, dfa, regex);
            		if(clusters.containsKey(explanation)) {
            			Map<String, Boolean> existing_cluster = clusters.get(explanation);
            			existing_cluster.putAll(cluster);
            			clusters.put(explanation, existing_cluster);
            		} else {
            			Map<String, Boolean> new_cluster = new HashMap<String, Boolean>();
            			new_cluster.putAll(cluster);
            			clusters.put(explanation, new_cluster);
            		}
            	}
        	}
		}

		if(clusters.size() > 1 && clusters.containsKey("Positive examples only")) {
			clusters.remove("Positive examples only");
		}
		return clusters;
	}

	static private Map<String, Map<String, Boolean>> generateSimilarExamples(String regex, String[] examples) {

		Map<String, Map<String, Boolean>> clusters = new LinkedHashMap<String, Map<String, Boolean>>();
		Automaton dfa = regexToDFA(regex);
		for(String example : examples) {
			Map<String, Map<String, Boolean>> map = generateExamples(example, regex,dfa);
			for(String explanation : map.keySet()) {
				Map<String, Boolean> cluster = map.get(explanation);
				if(clusters.containsKey(explanation)) {
					Map<String, Boolean> existingCluster = clusters.get(explanation);
					existingCluster.putAll(cluster);
					clusters.put(explanation, existingCluster);
				} else {
					Map<String, Boolean> new_cluster = new HashMap<String, Boolean>();
					new_cluster.putAll(cluster);
					clusters.put(explanation, new_cluster);
				}
			}
		}
		if(clusters.size() == 1
		&& clusters.containsKey("Positive examples only")) {
			String s = "We didn't find any negative examples. It seems this regex can accept any string. " +
					"Do you want to double check some corner cases instead?";
			Map<String, Boolean> cluster = clusters.get("Positive examples only");
			clusters.clear();
			clusters.put(s, cluster);
		} else {
			if(clusters.containsKey("Positive examples only")) {
				clusters.remove("Positive examples only");
			}
		}

		return clusters;
	}
	public static JSONObject generateExamplesJson2(String regex, String[] examplesList) {
		System.out.println(regex);
		Automaton dfa = regexToDFA(regex);
		Map<String, Map<String, Boolean>> clusters = generateSimilarExamples(regex, examplesList);
		JSONObject result = new JSONObject();
		int clusterIndex = 0;
		for (Map.Entry<String, Map<String, Boolean>> entry : clusters.entrySet()) {
			String explanation = entry.getKey();
			Map<String, Boolean> examples = entry.getValue();
			JSONObject existingCluster = result.optJSONObject(Integer.toString(clusterIndex));
			if (existingCluster == null) {
				existingCluster = new JSONObject();
				result.put(Integer.toString(clusterIndex), existingCluster);
				clusterIndex++;
			}
			existingCluster.put("explanation", explanation);
			JSONArray exampleArr = new JSONArray();
			int exampleIndex = 0;
			for (Map.Entry<String, Boolean> exampleEntry : examples.entrySet()) {
				String exampleString = exampleEntry.getKey();
				Boolean resultBool = exampleEntry.getValue();
				JSONObject exampleData = new JSONObject();
				exampleData.put("example", exampleString);
				exampleData.put("Result", resultBool);
				if (!resultBool) {
					int index = getIndexOfFailure(dfa, exampleString);
					exampleData.put("index", index);
				}
				exampleArr.put(exampleIndex, exampleData);
				exampleIndex++;
			}
			existingCluster.put("examples", exampleArr);
			existingCluster.put("num_of_examples", exampleIndex);
		}
		return result;
	}



	public static JSONObject generateExamplesJson(String example, String regex) {
		Automaton dfa = regexToDFA(regex);
		Map<String, Map<String, Boolean>> clusters = generateExamples(example, regex,dfa);
		JSONObject result = new JSONObject();
		int explanationIndex = 0;
		for (Map.Entry<String, Map<String, Boolean>> entry : clusters.entrySet()) {
			String explanation = entry.getKey();
			Map<String, Boolean> examples = entry.getValue();
			JSONObject existingCluster = result.optJSONObject(Integer.toString(explanationIndex));
			if (existingCluster == null) {
				existingCluster = new JSONObject();
				result.put(Integer.toString(explanationIndex), existingCluster);
				explanationIndex++;
			}
			JSONObject exampleObj = new JSONObject();
			int exampleIndex = 0; // reset exampleIndex for each cluster
			for (Map.Entry<String, Boolean> exampleEntry : examples.entrySet()) {
				String exampleString = exampleEntry.getKey();
				Boolean resultBool = exampleEntry.getValue();
				JSONObject exampleData = new JSONObject();
				exampleData.put("example", exampleString);
				exampleData.put("Result", resultBool);
				if (!resultBool) {
					int index = getIndexOfFailure(dfa, exampleString);
					exampleData.put("index", index);
				}
				exampleObj.put(Integer.toString(exampleIndex), exampleData);
				exampleIndex++;
			}
			existingCluster.put("explanation", explanation);
			existingCluster.put("examples", exampleObj);
		}
		return result;
	}
static	private int getIndexOfFailure(Automaton a, String s) {
		State p = a.getInitialState();
		int index = 0;
		for (int i = 0; i < s.length(); i++) {
			State q = p.step(s.charAt(i));
			if (q == null) {
				break;
			} else if (i == s.length() - 1 && !q.isAccept()) {
				break;
			}
			index = i + 1;
			p = q;
		}
		return index;
	}

	public static void main(String[] args) {
		if (args.length != 2) {
			System.out.println(args[0]);
			System.out.println(args[1]);
			System.out.println("Please provide an example and a regular expression.");
			return;
		}
		String exampls = args[0];
		String regex = args[1];
		// String example = args[0];
		// String regex = args[1];
	//	System.out.println(args[1]);
		//JSONObject result = generateExamplesJson2(regex, exampls);
		JSONObject result = generateExamplesJson(exampls, regex);
		System.out.println(result.toString(2));
	}
}

