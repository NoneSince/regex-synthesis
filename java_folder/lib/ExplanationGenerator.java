import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import dk.brics.automaton.Automaton;
import dk.brics.automaton.State;
import dk.brics.automaton.Transition;
public class ExplanationGenerator {
	public static String getOrdinalNum(int i) {
		switch (i) {
			case 1:
				return "1st";
			case 2:
				return "2nd";
			case 3:
				return "3rd";
			default:
				return i + "th";
		}
	}
	// Currently we only support generate explanations for negative examples
	public static String generateExplanation(String input, Automaton automaton, String regex) {
		String explanation = "";
		if (input.isEmpty()) {
			explanation = "The selected regular expression requires a non-empty input string.";
			return explanation;
		}
		State currentState = automaton.getInitialState();
		for (int i = 0; i < input.length(); i++) {
			State nextState = currentState.step(input.charAt(i));
			if (nextState == null) {
				if (currentState.isAccept() && currentState.getTransitions().isEmpty()) {
					explanation = "The input string is too long.";
				} else {
					String acceptedRange = getAcceptedCharsFromState(currentState);
					if (acceptedRange.startsWith("not ")) {
						String rejectedChar = acceptedRange.substring(4);
						if (rejectedChar.length() == 1 && regex.contains("concat(<" + rejectedChar + ">,<" + rejectedChar + ">)")) {
							explanation = "The input string contains the repeated character " + rejectedChar + ".";
						} else {
							explanation = "The " + getOrdinalNum(i + 1) + " character of the input string must not be " + rejectedChar + ".";
						}
					} else {
						explanation = "The " + getOrdinalNum(i + 1) + " character of the input string must be " + acceptedRange + ".";
					}
				}
				break;
			} else if (i == input.length() - 1 && !nextState.isAccept()) {
				List<Transition> acceptingTransitions = new ArrayList<>();
				for (Transition transition : nextState.getTransitions()) {
					if (transition.getDest().isAccept()) {
						acceptingTransitions.add(transition);
					}
				}
				if (acceptingTransitions.isEmpty()) {
					String acceptedRange = getAcceptedCharsFromState(nextState);
					if (acceptedRange.startsWith("not ")) {
						String rejectedChar = acceptedRange.substring(4);
						explanation = "The selected regular expression requires more characters. The next character cannot be " + rejectedChar + ".";
					} else {
						explanation = "The selected regular expression requires more characters. The next character must be " + acceptedRange + ".";
					}
				} else {
					if (regex.contains("contain")) {
						explanation = "The selected regular expression requires the input string to contain " + getAcceptedCharsInTransitions(acceptingTransitions) + ".";
					} else if (regex.contains("endwith")) {
						explanation = "The selected regular expression requires the input string to end with " + getAcceptedCharsInTransitions(acceptingTransitions) + ".";
					} else {
						String acceptedRange = getAcceptedCharsFromState(nextState);
						if (acceptedRange.startsWith("not ")) {
							String rejectedChar = acceptedRange.substring(4);
							explanation = "The selected regular expression requires more characters. The next character cannot be " + rejectedChar + ".";
						} else {
							explanation = "The selected regular expression requires more characters. The next character can be " + acceptedRange + ".";
						}
					}
				}
				break;
			}
			currentState = nextState;
		}
		return explanation;
	}
	public static String generateExplanation(String example, Automaton[] automata, String[] regexes, Boolean[] results) {
		StringBuilder explanation = new StringBuilder();
		Map<String, List<String>> regexExplanationMap = new HashMap<>();
		for (int i = 0; i < results.length; i++) {
			if (!results[i]) {
				String regex = regexes[i];
				String regexExplanation = ExplanationGenerator.generateExplanation(example, automata[i], regex);

				List<String> regexList = regexExplanationMap.getOrDefault(regexExplanation, new ArrayList<>());
				regexList.add(regex);
				regexExplanationMap.put(regexExplanation, regexList);
			}
		}
		for (String regexExplanation : regexExplanationMap.keySet()) {
			List<String> regexList = regexExplanationMap.get(regexExplanation);
			if (regexExplanation.contains("the selected regex") && regexList.size() > 1) {
				regexExplanation = regexExplanation.replace("the selected regex", "these regexes");
			} else if (regexExplanation.contains("the selected regex")) {
				regexExplanation = regexExplanation.replace("the selected regex", "this regex");
			}
			explanation.append("Examples rejected by ");
			explanation.append(String.join(" and ", regexList));
			explanation.append(": ");
			explanation.append(regexExplanation);
			explanation.append(". ");
		}
		if (explanation.length() > 2) {
			explanation.deleteCharAt(explanation.length() - 1);
		}
		return explanation.toString();
	}

	private static String getAcceptedCharsFromState(State s) {
		if(s.getTransitions().isEmpty()) {
			return "";
		}
		List<Transition> trans = s.getSortedTransitions(true);
		String range = getAcceptedCharsInTransitions(trans);
		return range;
	}
	private static String getAcceptedCharsInTransitions(List<Transition> trans) {
		StringBuilder sb = new StringBuilder();
		boolean isAnyChar = true;
		char space = ' ';
		char tilde = '~';
		for (Transition t : trans) {
			char start = t.getMin();
			char end = t.getMax();
			if (start > end) {
				// Ignore invalid transitions
				continue;
			}
			if (start < space) {
				start = space;
			}
			if (end > tilde) {
				end = tilde;
			}
			if (isAnyChar) {
				isAnyChar = false;
				if (start == space && end == tilde) {
					// All printable ASCII characters are accepted
					sb.append("any printable ASCII character");
					continue;
				}
			}
			if (start == end) {
				sb.append(start == space ? "empty space" : start);
			} else if (end - start == 1) {
				sb.append(start == space ? "empty space" : start).append(" or ").append(end);
			} else if (end - start == 2) {
				sb.append(start == space ? "empty space" : start).append(" or ").append((char) (start + 1)).append(" or ").append(end);
			} else {
				sb.append(start == space ? "empty space" : start).append(" to ").append(end);
			}
			sb.append(" or ");
		}
		if (sb.length() > 0) {
			sb.setLength(sb.length() - 4); // Remove the last " or "
		}
		if (isAnyChar) {
			sb.append("no printable ASCII characters");
		}
		return sb.toString();
	}
}
