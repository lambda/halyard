# An extremely primitive web server.  We use this to test various
# networking APIs in Halyard.
#
# To run:
#   sudo gem install sinatra
#   ruby http-test-server.rb

require 'rubygems'
require 'sinatra'
require 'json'
require 'cgi'

get '/' do
  "This is a primitive web server used for testing Halyard."
end

get '/not-found' do
  status 404
  "This page doesn't exist."
end

get '/hello' do
  "Hello!\n" * (params[:count] || "1").to_i
end

get '/upload' do
  builder do |xml|
    xml.form :method => "post", :action => "/upload" do
      xml.input :type => "text", :name => "message", :value => ""
      xml.input :type => "submit"
    end
  end
end

post '/upload' do
  "post: #{request.body.read}"
end

get '/headers/:name' do 
  content_type 'text/plain'
  env["HTTP_#{params[:name].upcase.gsub(/-/, '_')}"]
end

get '/add' do
  content_type :json
  (params[:x].to_i + params[:y].to_i).to_json
end

post '/echo' do
  content_type request.content_type
  request.body
end


#==========================================================================
#  Simulated HACP LMS API
#==========================================================================

require 'test/unit/assertions'

Sinatra::Application.send(:include, Test::Unit::Assertions)

HACP_UUID = "44463f20-b4c6-4a3e-abf6-b942d010deb3" unless defined?(HACP_UUID)
HACP_SESSION_ID = "#{HACP_UUID}:123:4567" unless defined?(HACP_SESSION_ID)

post '/hacp/register' do
  assert_equal HACP_UUID, params[:uuid]
  assert_equal "J. Student", params[:name]
  assert_equal "12345", params[:student_id]

  content_type :json
  {}.to_json
end

post '/hacp/new_session' do
  assert_equal HACP_UUID, params[:uuid]
  
  content_type :json
  { 'aicc_url' => "http://localhost:4567/hacp",
    'aicc_sid' => HACP_SESSION_ID }.to_json
end

post '/hacp' do
  assert_equal HACP_SESSION_ID, params[:session_id]
  assert_equal "4.0", params[:version]

  content_type :text
  case params[:command].downcase
  when "getparam"
    <<EOD
error=0
error_text=Successful
aicc_data=
[Core]
Student_ID = 12345
Student_Name = J.+Student
Lesson_Location = 
Credit = credit
Lesson_Status = not+attempted
Score = 
Time = 00%3a00%3a00

[Core_Lesson]

[Core_Vendor]
EOD
  when "putparam"
    assert_equal <<EOD, params[:aicc_data]
[Core]
Lesson_Location=%2fstart
Lesson_Status=incomplete
Score=72%2c100
Time=00%3a05%3a00
J_ID.1=%2fpart1
J_Status.1=completed
J_ID.2=%2fpart2
J_Status.2=incomplete
[Core_Lesson]
data%0a%5bfoo%5d
EOD
    <<EOD
error=0
error_text=Successful
EOD
  else
    raise ArgumentError, "Unkown HACP command: #{params[:command]}"
  end
end


#==========================================================================
#  Simulated HACP LMS API #2
#==========================================================================
#  We use this second version of the HACP interface to test whether the
#  high-level API makes all the protocol calls in the correct order.

# Strip invalid characters from UUID to make student ID.
HACP_STUDENT_ID = HACP_UUID.gsub('-', '') unless defined? HACP_STUDENT_ID

$hacp2_log ||= []
$hacp2_failing_commands ||= {}

# If 'command' has been marked as failing, abort with 500 Internal Error.
def fail_if_requested command
  halt 500 if $hacp2_failing_commands[command]
end

post '/hacp2/reset_test_state' do
  $hacp2_log = []
  $hacp2_failing_commands = {}
  content_type :text
  "Log is reset."
end

# Trigger artificial failures for the specified command.
post '/hacp2/fail_on' do
  $hacp2_failing_commands[params[:command]] = true
end

# Stop triggering artificial failures for the specified command.
post '/hacp2/succeed_on' do
  $hacp2_failing_commands.delete(params[:command])
end

get '/hacp2/log' do
  content_type :text
  $hacp2_log.join(' ')
end

post '/hacp2/register' do
  fail_if_requested 'register'

  # If this is our standard test user, verify all the other fields and log
  # this request.
  if params[:uuid] == HACP_UUID
    assert_equal "J. Student", params[:name]
    assert_equal HACP_STUDENT_ID, params[:student_id]
    $hacp2_log << "register"
  end

  content_type :json
  {}.to_json
end

post '/hacp2/new_session' do
  fail_if_requested 'new_session'

  # If this is our standard test user, log this request.
  if params[:uuid] == HACP_UUID
    $hacp2_log << "new_session"
  end
  
  content_type :json
  { 'aicc_url' => "http://localhost:4567/hacp2.1",
    'aicc_sid' => "#{params[:uuid]}:123:4567" }.to_json
end

post '/hacp2.1' do
  # If this is our standard test user, verify all the other fields and log
  # this request.
  should_validate_and_log = (HACP_SESSION_ID == params[:session_id])

  command = params[:command].downcase
  fail_if_requested command

  content_type :text
  case command
  when "getparam"
    $hacp2_log << "GetParam" if should_validate_and_log
    <<"EOD"
error=0
error_text=Successful
aicc_data=
[Core]
Student_ID = #{CGI.escape(HACP_STUDENT_ID)}
Student_Name = J.+Student
Lesson_Location = 
Credit = credit
Lesson_Status = not+attempted
Score = 
Time = 00%3a00%3a00

[Core_Lesson]
EOD
  when "putparam"
    if should_validate_and_log
      assert_equal <<EOD, params[:aicc_data]
[Core]
lesson_location=%2fnetworking%2ftests%2fhacp
[Core_Lesson]

EOD
      $hacp2_log << "PutParam"
    end
  else
    raise ArgumentError, "Unkown HACP command: #{params[:command]}"
  end
end
