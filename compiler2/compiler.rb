class Compiler
  
  class Error < RuntimeError
  end
  
  class Node
    Mapping = {}
    
    def self.kind(name=nil)
      return @kind unless name
      Mapping[name] = self
      @kind = name
    end
    
    def self.create(compiler, sexp)
      sexp.shift
      
      node = new(compiler)
      args = node.consume(sexp)

      begin
        if node.respond_to? :normalize
          node = node.normalize(*args)
        else
          node.args(*args)
        end
      rescue ArgumentError => e
        raise ArgumentError, "#{kind} (#{self}) takes #{args.size} argument(s): passed #{args.inspect} (#{e.message})", e.context
      end
      
      return node
    end
    
    def initialize(compiler)
      @compiler = compiler
    end
    
    def convert(x)
      @compiler.convert_sexp(x)
    end
    
    def consume(sexp)
      # This lets nil come back from convert_sexp which means
      # leave it out of the stream. This is primarily so that
      # expressions can be optimized away and wont be seen at
      # all in the output stream.
      out = []
      sexp.each do |x|        
        if x.kind_of? Array
          v = @compiler.convert_sexp(x)
          out << v unless v.nil?
        else
          out << x
        end
      end
      
      return out
    end
    
    def args
    end
    
    def position?(tag)
      @compiler.position?(tag)
    end
    
    def position(tag, val=true, &b)
      @compiler.position(tag, val, &b)
    end
    
    def inspect
      kind = self.class.kind
      if kind
        prefix = "Compiler:#{self.class.kind}"
      else
        prefix = self.class.name
      end
      super(prefix, [:@compiler])
    end
    
    def is?(clas)
      self.kind_of?(clas)
    end
  end
  
  def initialize(gen_klass)
    @position = {}
    @generator = gen_klass
    @call_plugins = []
    
    @file = "(unknown)"
    @line = 0
    
    load_plugins
  end
  
  attr_reader :generator, :call_plugins
  
  def set_position(file, line)
    @file, @line = file, line
  end
  
  def load_plugins
    require 'compiler2/plugins'
    @call_plugins << Compiler::Plugins::BlockGiven.new(self)
    @call_plugins << Compiler::Plugins::PrimitiveDeclaration.new(self)
    @call_plugins << Compiler::Plugins::FastMathOperators.new(self)
  end
  
  def inspect
    "#<#{self.class}>"
  end
    
  def convert_sexp(sexp)
    return nil if sexp.nil?
    
    klass = Compiler::Node::Mapping[sexp.first]
    
    raise Error, "Unable to resolve #{sexp.first}" unless klass

    return klass.create(self, sexp)
  end
  
  def into_script(sexp)
    begin
      convert_sexp([:script, sexp])
    rescue Object => e
      puts "Complication error detected: #{e.message}"
      puts "   near #{@file}:#{@line}"
      puts
      puts e.backtrace.show
    end
  end
  
  def position?(tag)
    @position[tag]
  end
  
  def position(tag, val=true)
    cur = @position[tag]
    @position[tag] = val
    begin
      yield
    ensure
      @position[tag] = cur
    end
  end
  
  def fresh_position
    cur = @position
    @position = {}
    begin
      yield
    ensure
      @position = cur
    end
  end
end

require 'nodes'
require 'local'
